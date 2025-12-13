import requests
from bs4 import BeautifulSoup
import re
import json
from datetime import datetime
import time

START_URL = "https://tpirepguide.com/?p=9773"
MAX_PAGES = 20000
REQUEST_DELAY = 0.1

# -----------------------------------------------------------
# DATE PARSER
# -----------------------------------------------------------
def clean_and_parse_date(date_text: str):
    raw = date_text.strip()
    cleaned = re.sub(r'(\d+)(st|nd|rd|th)', r'\1', raw)
    iso_date = None
    try:
        dt = datetime.strptime(cleaned, "%B %d, %Y")
        iso_date = dt.date().isoformat()
    except ValueError:
        pass
    return raw, iso_date

# -----------------------------------------------------------
# FOOTER EXTRACTOR
# -----------------------------------------------------------
def extract_footer_info(soup: BeautifulSoup):
    footer = soup.select_one("div.post-footer")
    if not footer:
        return None, None, []

    footer_text = footer.get_text(" ", strip=True)
    date_part = footer_text.split("|", 1)[0].strip()
    raw_date, iso_date = clean_and_parse_date(date_part)
    categories = [a.get_text(strip=True) for a in footer.select("a")]
    return raw_date, iso_date, categories

# -----------------------------------------------------------
# **UPDATED SHOWCASE SHOWDOWN EXTRACTOR**
# -----------------------------------------------------------
def extract_showcase_showdowns(soup: BeautifulSoup):
    container = (
        soup.select_one("div.post-bodycopy")
        or soup.select_one("div.post")
        or soup
    )

    text = container.get_text("\n", strip=False)
    text = text.replace("\u00A0", " ")
    lines = text.splitlines()

    showdowns = []
    current_label = None
    buffer = []
    scoreboard_started = False

    # --- Flush function: normalize whitespace before saving ---
    def flush():
        nonlocal current_label, buffer, scoreboard_started
        if current_label and buffer:
            body = "\n".join(buffer).strip()
            if body:
                body = re.sub(r"\s+", " ", body).strip()
                showdowns.append({
                    "label": current_label,
                    "text": body,
                })
        current_label = None
        buffer = []
        scoreboard_started = False

    # --- Header recognition logic ---
    def is_showdown_header(header_norm: str) -> bool:
        if header_norm.startswith("showcase showdown"):
            return True
        if header_norm.startswith("showdown"):
            return True
        # Handles ep formats like "SHOWCASE #1" (wheel, NOT final Showcases)
        if re.match(r"^showcase\s*#\d+\b", header_norm):
            return True
        return False

    # --- Scoreboard recognizers ---
    def is_scoreboard_start(s: str) -> bool:
        return bool(re.match(r"^\$?\s*[\d,]", s))

    def is_scoreboard_continuation(s: str) -> bool:
        s_strip = s.strip()
        if not s_strip:
            return False

        if re.fullmatch(r"[-+]?\d+(\.\d+)?", s_strip):
            return True
        if s_strip in {"+", "-"}:
            return True
        if re.match(r"through to the showcases?", s_strip, re.I):
            return True
        if re.match(r"(bonus spin|spin[- ]off|busted|dollar)", s_strip, re.I):
            return True

        return False

    # ---------------------------------------------------------
    # MAIN LINE LOOP
    # ---------------------------------------------------------
    for line in lines:
        stripped = line.strip()
        header_norm = re.sub(r"\s+", " ", stripped).lower()

        # End of wheel showdowns: hit the "SHOWCASES" section
        if header_norm.startswith("showcases"):
            flush()
            break

        # Start of a new showdown?
        if is_showdown_header(header_norm):
            flush()
            current_label = stripped or "Showcase Showdown"
            buffer = []
            scoreboard_started = False
            continue

        # If we are currently inside a showdown:
        if current_label is not None:

            # --- If scoreboard started ---
            if scoreboard_started:

                # Do NOT flush on blank lines
                if not stripped:
                    continue

                # Separator "***"
                if re.match(r"^\* \* \*$", stripped):
                    flush()
                    continue

                # More scoreboard content?
                if is_scoreboard_start(stripped) or is_scoreboard_continuation(stripped):
                    buffer.append(stripped)
                else:
                    # First non-scoreboard line terminates this showdown
                    flush()

            else:
                # We haven't started scoreboard yet
                if not stripped:
                    continue
                if is_scoreboard_start(stripped):
                    scoreboard_started = True
                    buffer.append(stripped)

    flush()
    return showdowns

# -----------------------------------------------------------
# NEXT-PAGE RESOLUTION
# -----------------------------------------------------------
def find_next_url(soup: BeautifulSoup, current_url: str):
    next_link = soup.select_one("div.post-pagination a.page-numbers")
    if next_link and "href" in next_link.attrs:
        return requests.compat.urljoin(current_url, next_link["href"])

    rel_next = soup.find("a", rel="next")
    if rel_next and "href" in rel_next.attrs:
        return requests.compat.urljoin(current_url, rel_next["href"])

    newer_link = soup.select_one("div.navigation-top div.newer a")
    if newer_link and "href" in newer_link.attrs:
        return requests.compat.urljoin(current_url, newer_link["href"])

    return None

# -----------------------------------------------------------
# SCRAPE A SINGLE EPISODE
# -----------------------------------------------------------
def scrape_episode(url: str):
    print(f"\nFetching: {url}")
    resp = requests.get(url, headers={"User-Agent": "Mozilla/5.0"})
    resp.raise_for_status()

    soup = BeautifulSoup(resp.text, "html.parser")
    title_tag = soup.select_one("div.post-headline h1") or soup.find("title")
    episode_title = title_tag.get_text(strip=True) if title_tag else None

    raw_date, iso_date, categories = extract_footer_info(soup)
    showdowns = extract_showcase_showdowns(soup)
    next_url = find_next_url(soup, url)

    if not showdowns:
        print(f"  [info] No Showcase Showdowns detected for {episode_title or url}")

    return {
        "url": url,
        "episode_title": episode_title,
        "raw_date": raw_date,
        "iso_date": iso_date,
        "categories": categories,
        "showcase_showdowns": showdowns,
        "next_url": next_url,
    }

# -----------------------------------------------------------
# CRAWLER
# -----------------------------------------------------------
def crawl_episodes(start_url: str, max_pages: int):
    episodes = []
    current_url = start_url
    start_time = time.time()

    for i in range(max_pages):
        try:
            data = scrape_episode(current_url)
        except KeyboardInterrupt:
            print("\n\n=== CTRL-C detected during crawl — stopping early ===\n")
            break  # <-- break the loop but KEEP everything already in `episodes`
        except Exception as e:
            print(f"Error on {current_url}: {e}")
            break

        episodes.append(data)

        # ---- Enhanced Logging Block ----
        elapsed = time.time() - start_time
        elapsed_str = time.strftime("%H:%M:%S", time.gmtime(elapsed))

        print(f"\n=== PAGE {i+1}/{max_pages} ===")
        print(f"Episode: {data.get('episode_title')}")
        print(f"Date:    {data.get('raw_date')} (ISO: {data.get('iso_date')})")
        print(f"URL:     {current_url}")
        print(f"Elapsed: {elapsed_str}")
        print(f"Showdowns found: {len(data.get('showcase_showdowns', []))}")

        # ETA estimate
        if i > 0:
            avg_per_page = elapsed / (i + 1)
            remaining = max_pages - (i + 1)
            eta_seconds = avg_per_page * remaining
            eta_str = time.strftime("%H:%M:%S", time.gmtime(eta_seconds))
            print(f"ETA:     ~{eta_str}")

        print("==========================\n")

        # Check next page
        next_url = data.get("next_url")
        if not next_url:
            print("No next page found; stopping.")
            break

        current_url = next_url

        try:
            time.sleep(REQUEST_DELAY)
        except KeyboardInterrupt:
            print("\n\n=== CTRL-C detected during sleep — stopping early ===\n")
            break  # <-- KEEP accumulated episodes

    return episodes


# -----------------------------------------------------------
# MAIN
# -----------------------------------------------------------
if __name__ == "__main__":
    episodes_data = []
    try:
        # Start crawling
        episodes_data = crawl_episodes(START_URL, MAX_PAGES)

    except KeyboardInterrupt:
        print("\n\n=== CTRL-C detected! Saving partial results... ===\n")

    finally:
        # Whether crawl completed or was interrupted, save what we have
        full_output_file = "tpir_episodes_full.json"
        with open(full_output_file, "w", encoding="utf-8") as f:
            json.dump(episodes_data, f, ensure_ascii=False, indent=2)

        showcases_only = []
        for ep in episodes_data:
            showcases_only.append({
                "episode_title": ep.get("episode_title"),
                "raw_date": ep.get("raw_date"),
                "iso_date": ep.get("iso_date"),
                "showcase_showdowns": ep.get("showcase_showdowns", []),
            })

        output_file = "tpir_showcases_only.json"
        with open(output_file, "w", encoding="utf-8") as f:
            json.dump(showcases_only, f, ensure_ascii=False, indent=2)

        print(f"\nSaved {len(episodes_data)} episodes to {full_output_file}")
        print(f"Saved {len(showcases_only)} showcase-only entries to {output_file}")
        print("\n=== Done. Exiting cleanly. ===\n")