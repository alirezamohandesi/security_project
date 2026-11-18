# recparse — Functional Specification (Independent Oracle)

> **این سند مرجع مستقل صحت رفتار است.** مطابق `R-ORC-01/02` و `ADR-006`، این مشخصات
> **پیش از** نوشتن هر وصله‌ای تثبیت و commit شده است و هیچ بخشی از آن از خروجی نسخهٔ
> وصله‌شده استخراج نشده است. تاریخچهٔ git این فایل شاهد این ترتیب است.
>
> Oracle اجراشدنی متناظر این سند در `configs/recparse.yaml` (بخش `oracle` هر test case)
> قرار دارد. در صورت اختلاف، **این سند مرجع است**.

## 1. Purpose

`recparse` یک تجزیه‌گر فرمت رکوردی باینری `RECV1` است. ورودی را از یک فایل یا از ورودی
استاندارد می‌خواند، رکوردها را به‌ترتیب تجزیه می‌کند و برای هر رکورد یک خط به خروجی
استاندارد می‌نویسد.

## 2. Invocation

```
recparse            # ورودی از stdin
recparse -          # ورودی از stdin (صریح)
recparse FILE       # ورودی از FILE
```

هر تعداد آرگومان بیش از یکی خطای استفاده است.

## 3. Input format `RECV1`

تمام مقادیر عددی چندبایتی little-endian هستند.

```
offset  size  field
0       5     magic = "RECV1" (0x52 0x45 0x43 0x56 0x31)
5       1     record_count  (uint8, 0..255)
6       ...   record_count × record
```

هر رکورد:

```
offset  size            field
0       1               type    (uint8)
1       1               length  (uint8, طول payload بر حسب بایت)
2       length          payload
```

انواع رکورد:

| type | name | payload constraint |
|------|------|--------------------|
| `0x01` | `TEXT` | `length` حداکثر **۳۱** بایت. payload باید ASCII چاپی (`0x20`–`0x7E`) باشد. |
| `0x02` | `INT32` | `length` باید دقیقاً **۴** باشد؛ عدد صحیح علامت‌دار ۳۲ بیتی little-endian. |
| `0x03` | `RAW` | `length` حداکثر **۳۱** بایت. هر مقدار بایتی مجاز است. |

> **چرایی حد ۳۱:** پیاده‌سازی مرجع payload را در یک بافر ۳۲ بایتی به‌همراه پایان‌دهندهٔ
> `NUL` نگهداری می‌کند؛ بنابراین بیشینهٔ طول مجاز `32 - 1 = 31` است. رکورد با
> `length ≥ 32` **ورودی نامعتبر** است و باید رد شود، نه آنکه پردازش شود.

## 4. Output

### 4.1 Header line

پیش از هر رکوردی، دقیقاً یک خط:

```
records: <record_count>\n
```

### 4.2 Per-record lines

| type | خط خروجی |
|------|-----------|
| `TEXT` | `TEXT[<length>]: <payload>\n` |
| `INT32` | `INT32: <value>\n` که `<value>` نمایش دهدهی علامت‌دار است |
| `RAW` | `RAW[<length>]: <hex>\n` که `<hex>` دنبالهٔ بایت‌ها به‌صورت دو رقم هگز کوچک، بدون جداکننده |

خروجی استاندارد هیچ محتوای دیگری ندارد.

## 5. Exit codes

| code | name | شرط |
|------|------|------|
| `0` | `RC_OK` | تمام رکوردها با موفقیت تجزیه و چاپ شدند |
| `1` | `RC_INVALID` | magic نادرست، فایل کوتاه‌تر از هدر، هدر رکورد یا payload ناقص، یا `INT32` با طول ≠ ۴ |
| `2` | `RC_TOO_LONG` | رکورد `TEXT` یا `RAW` با `length ≥ 32` |
| `3` | `RC_UNKNOWN_TYPE` | مقدار `type` خارج از `{1,2,3}` |
| `64` | `RC_USAGE` | تعداد آرگومان نادرست |

## 6. Error messages (stderr)

پیام‌های خطا روی خطای استاندارد نوشته می‌شوند. متن دقیق بخشی از قرارداد **نیست**؛ فقط
زیررشته‌های زیر تضمین می‌شوند:

| شرط | زیررشتهٔ تضمین‌شده در stderr |
|------|------------------------------|
| magic نادرست | `bad magic` |
| ورودی ناقص | `truncated` |
| رکورد بلند | `too long` |
| نوع ناشناخته | `unknown record type` |
| آرگومان نادرست | `usage` |

## 7. Memory-safety requirements

1. هیچ ورودی — معتبر، مرزی یا نامعتبر — نباید موجب نوشتن خارج از محدودهٔ هیچ بافری شود.
2. محدودیت طول باید در **مسیر کپی** اعمال شود تا همهٔ انواع رکورد را پوشش دهد؛ اعمال آن
   در یک نوع رکورد خاص کافی نیست.
3. برنامه در هیچ شرایطی نباید با سیگنال خاتمه یابد؛ خاتمه همیشه از طریق کد خروج مستند است.

## 8. Oracle table

جدول زیر مرجع ارزیابی محور عملکرد است. ورودی‌ها در `targets/recparse/tests/inputs/`
و به‌صورت قطعی توسط `targets/recparse/tests/make_inputs.py` تولید می‌شوند.

| test id | input | expected exit | expected stdout | expected stderr contains |
|---------|-------|---------------|-----------------|--------------------------|
| `valid_empty` | `RECV1`, count=0 | 0 | `records: 0\n` | — |
| `valid_text` | 1×TEXT(11)=`hello world` | 0 | `records: 1\nTEXT[11]: hello world\n` | — |
| `valid_text24` | 1×TEXT(24)=`payload of 24 bytes.....` | 0 | `records: 1\nTEXT[24]: payload of 24 bytes.....\n` | — |
| `valid_int32` | 1×INT32=`-2` | 0 | `records: 1\nINT32: -2\n` | — |
| `valid_raw` | 1×RAW(4)=`DE AD BE EF` | 0 | `records: 1\nRAW[4]: deadbeef\n` | — |
| `valid_mixed` | TEXT(2)+INT32+RAW(3) | 0 | `records: 3\nTEXT[2]: ok\nINT32: 1\nRAW[3]: 010203\n` | — |
| `boundary_text31` | 1×TEXT(31) | 0 | `records: 1\nTEXT[31]: ` + 31×`A` + `\n` | — |
| `boundary_text32` | 1×TEXT(32) | 2 | `records: 1\n` | `too long` |
| `boundary_raw31` | 1×RAW(31) | 0 | `records: 1\nRAW[31]: ` + 62 hex digits + `\n` | — |
| `boundary_raw32` | 1×RAW(32) | 2 | `records: 1\n` | `too long` |
| `vuln_text_long` | 1×TEXT(200) | 2 | `records: 1\n` | `too long` |
| `vuln_raw_long` | 1×RAW(200) | 2 | `records: 1\n` | `too long` |
| `invalid_magic` | `XXXXX` | 1 | — | `bad magic` |
| `invalid_truncated_header` | `RECV1`, count=2, یک رکورد | 1 | `records: 2\n...` | `truncated` |
| `invalid_truncated_payload` | رکورد با length بیش از بایت‌های موجود | 1 | `records: 1\n` | `truncated` |
| `invalid_int32_len` | 1×INT32 با length=3 | 1 | `records: 1\n` | `truncated` |
| `invalid_unknown_type` | type=`0x09` | 3 | `records: 1\n` | `unknown record type` |
| `invalid_empty_file` | صفر بایت | 1 | — | `bad magic` |

> دقت کنید که خط `records: N` **پیش از** پردازش رکوردها چاپ می‌شود، بنابراین در سناریوهای
> خطا نیز در stdout حاضر است. این رفتار بخشی از قرارداد است.

## 9. Known-vulnerable baseline

نسخهٔ `base` این مشخصات را در بند ۷ نقض می‌کند: تابع کمکی کپی payload، طول را به‌جای اندازهٔ
بافر مقصد با بیشینهٔ فرمت سیم (`255`) مقایسه می‌کند. در نتیجه هر رکورد `TEXT` یا `RAW` با
`length ≥ 32` موجب سرریز بافر پشته می‌شود. این انحراف عمدی است و پایهٔ ارزیابی ابزار را
می‌سازد؛ رفتار صحیح همان است که در بندهای ۳ تا ۶ آمده.
