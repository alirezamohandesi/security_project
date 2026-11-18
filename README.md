# security_project — recparse

یک پارسر اسباب‌بازی به زبان C برای قالب باینری `RECV1` با یک آسیب‌پذیری
**stack buffer overflow** در تابع مشترک `copy_payload()` (فایل `src/record.c`).
شاخهٔ `main` نسخهٔ آسیب‌پذیر (baseline) است. هر توسعه‌دهنده وصلهٔ خود را در یک شاخهٔ جدا
push می‌کند و پایپ‌لاین GitHub Actions به‌صورت خودکار، با ابزار
[patch-validator](https://github.com/mohammaderfanali/patch-validator)، بررسی می‌کند که
آیا وصله آسیب‌پذیری را **کامل و درست** رفع کرده است یا نه.

## ساختار

```
src/                         کد C برنامه (main.c, record.c, record.h)
CMakeLists.txt               سیستم ساخت
SPECIFICATION.md             مشخصات قالب و رفتار مورد انتظار
tests/inputs/                ورودی‌های آزمون (valid / boundary / invalid / security)
tests/make_inputs.py         تولیدکنندهٔ ورودی‌ها
tests/run_functional.py      اجراکنندهٔ سریع آزمون‌های عملکردی
.patch-validator/recparse.yaml  پیکربندی و Oracleهای patch-validator
.github/workflows/ci.yml     پایپ‌لاین کامل
```

## شاخه‌های وصله

| شاخه | توسعه‌دهنده | تغییر | نتیجهٔ مورد انتظار |
|------|-------------|-------|---------------------|
| `fix/bound-copy-by-buffer` | Alice | محدود کردن کپی به اندازهٔ بافر مقصد در `copy_payload()` | ✅ `PASS` (exit 0) |
| `fix/reject-long-text` | Bob | فقط رکوردهای TEXT بلند رد می‌شوند | ❌ `VULN_REMAINS` (exit 10) — مسیر RAW هنوز سرریز می‌کند |
| `fix/strict-text-limit` | Carol | رفع درست + محدودیت ۱۶ بایتی برای TEXT | ❌ `FUNC_REGRESSION` (exit 20) — ورودی معتبر ۲۴ بایتی رد می‌شود |

## پایپ‌لاین

با هر push به هر شاخه‌ای غیر از `main` اجرا می‌شود (و دستی از تب Actions):

```
prepare ─┬─ static-analysis ───────────────────────────────┐
         ├─ build (gcc|clang × release|sanitized) ─┬─ functional-tests ─┤
         │                                         └─ patch-validator ──┤
         └──────────────────────────────────────────────────────────────┴─ verdict
```

| Job | کار | مسدودکننده؟ |
|-----|-----|-------------|
| `prepare` | تعیین `base` (merge-base با main) و `patched` (commit پوش‌شده)، تولید `patch.diff`، هشدار در صورت دست‌کاری تست‌ها | بله |
| `static analysis` | `cppcheck` و `gcc -fanalyzer` | خیر (اطلاعاتی) |
| `build` | ساخت با gcc و clang در حالت release و sanitized (ASan+UBSan)، با `-Werror` | بله |
| `functional tests` | اجرای باینری sanitized روی ورودی‌های valid/boundary/invalid و مقایسه با Oracle | بله |
| `patch-validator` | `doctor` → `check-config` → `hardening` → `validate` → بازاجرای suite رگرسیون → گزارش و artifact | بله |
| `verdict` | جمع‌بندی همهٔ نتایج در یک status check | بله |

**مرز اعتماد:** پیکربندی/Oracleها (`.patch-validator/`) و ورودی‌های آزمون (`tests/`) همیشه از
نسخهٔ پایه خوانده می‌شوند، نه از وصله؛ بنابراین یک وصله نمی‌تواند با تغییر تست‌های خودش قبول شود.

نسخهٔ ابزار در `PATCH_VALIDATOR_REF` در workflow روی یک commit مشخص pin شده است.

### کدهای خروج patch-validator

| کد | معنا |
|----|------|
| `0` | PASS — خطای پایه بازتولید شد و روی وصله دیده نشد، آزمون‌های عملکردی موفق |
| `10` | آسیب‌پذیری باقی مانده یا خطای حافظهٔ جدید |
| `20` | پسرفت عملکردی |
| `30` | خطای ساخت یا پیکربندی |
| `40` | غیرقطعی |

گزارش کامل (`report.md`) در خلاصهٔ job نمایش داده می‌شود و پوشهٔ کامل اجرا
(لاگ‌ها، crashها، بسته‌های بازتولید، ورودی‌های کوچک‌شده) به‌عنوان artifact با نام
`patch-validator-run` آپلود می‌شود.

## افزودن یک وصلهٔ جدید

```bash
git checkout main && git checkout -b fix/my-patch
# ویرایش src/record.c
git commit -am "parser: ..."
git push -u origin fix/my-patch
```

## اجرای محلی

```bash
cmake -S . -B build -DCMAKE_C_FLAGS="-fsanitize=address,undefined" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build
python3 tests/run_functional.py --binary build/recparse
```

```bash
pip install git+https://github.com/mohammaderfanali/patch-validator
patch-validator validate --config .patch-validator/recparse.yaml --base-revision main --patched-revision fix/bound-copy-by-buffer
```
