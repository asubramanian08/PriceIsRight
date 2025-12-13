import re
import os
from io import StringIO
import requests
import pandas as pd
from bs4 import BeautifulSoup

URL = "https://support.google.com/chromeosflex/answer/11513094?hl=en"

# --- fetch HTML ---
r = requests.get(URL, headers={"User-Agent": "Mozilla/5.0"}, timeout=30)
r.raise_for_status()
soup = BeautifulSoup(r.text, "lxml")

# --- helpers ---
def normalize_columns(cols):
    out = []
    for c in cols:
        cc = c.strip()
        cc = cc.replace("Model name","Model")
        cc = cc.replace("Current status","Status")
        cc = cc.replace("Certified since ChromeOS version","Since")
        cc = cc.replace("End of support","EndOfSupportYear")
        out.append(cc)
    return out

def clean_status(s):
    return (str(s)
        .replace("\xa0"," ")
        .strip()
        .replace("  "," "))

def strip_vendor_prefix(model: str, vendor: str) -> str:
    """Remove leading vendor name from model string if present."""
    if pd.isna(model) or pd.isna(vendor):
        return model
    s = str(model).strip()
    v = str(vendor).strip()
    if not v:
        return s

    ven = re.escape(v)
    # allow one or more vendor occurrences at start, followed by space/hyphen/underscore/colon
    pat = re.compile(rf"^(?:{ven})(?:\s+|[-–—_:])+(.+)$", re.IGNORECASE)
    changed = False
    while True:
        m = pat.match(s)
        if not m:
            break
        s = m.group(1).strip()
        changed = True

    if not s:  # don’t blank it out
        return str(model).strip()

    s = re.sub(r"\s{2,}", " ", s).strip()
    return s

rows = []

# --- parse vendor wrappers ---
wrappers = soup.select("div.zippy-wrapper")
print(f"Found vendor wrappers: {len(wrappers)}")

for w in wrappers:
    a = w.select_one("a.zippy")
    vendor = a.get_text(strip=True) if a else "Unknown"

    for tbl in w.select("table"):
        df = pd.read_html(StringIO(str(tbl)), flavor="lxml")[0]
        df.columns = normalize_columns(df.columns)

        if "Status" in df.columns:
            df["Status"] = df["Status"].map(clean_status)

        if "Since" in df.columns:
            df["Since"] = pd.to_numeric(df["Since"].replace("—", None),
                                        errors="coerce").astype("Int64")
        if "EndOfSupportYear" in df.columns:
            df["EndOfSupportYear"] = pd.to_numeric(df["EndOfSupportYear"],
                                                   errors="coerce").astype("Int64")

        df.insert(0, "Vendor", vendor)
        rows.append(df)

# --- combine all ---
final = pd.concat(rows, ignore_index=True)

# strip vendor prefix from Model
final["Model"] = final.apply(lambda r: strip_vendor_prefix(r["Model"], r["Vendor"]), axis=1)

# drop duplicates and sort
dedup_cols = ["Vendor","Model","Status","Since","EndOfSupportYear"]
final = final.drop_duplicates(subset=dedup_cols).sort_values(["Vendor","Model"]).reset_index(drop=True)

print("Total models:", len(final))
print("Unique vendors:", final["Vendor"].nunique())

# --- exports ---
final.to_csv("chromeos_flex_models_full.csv", index=False, encoding="utf-8")
final.to_json("chromeos_flex_models_full.json", orient="records", indent=2, force_ascii=False)

# optional per-vendor split
os.makedirs("vendors", exist_ok=True)
for vendor, dfv in final.groupby("Vendor", sort=True):
    safe = re.sub(r"[^A-Za-z0-9._-]+", "_", vendor)
    dfv.to_csv(f"vendors/{safe}.csv", index=False, encoding="utf-8")

# --- quick peek ---
print(final.sample(10, random_state=1))
