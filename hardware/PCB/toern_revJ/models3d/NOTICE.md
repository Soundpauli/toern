# RevJ third-party 3D model notices and redistribution review

Reviewed 2026-11-23. This notice records available evidence; it does not grant
permission on behalf of third-party rights holders. TŒRN's MIT, CC BY-NC and
exclusive commercial-sale statements do not relicense these third-party assets.

## KiCad models

The files under `KiCad/` are KiCad community library models. Their embedded
copyright and license notices are preserved. They use CC BY-SA 4.0 with KiCad's
electronic-design exception. The upstream [library license](KiCad/LICENSE.md)
and [full CC BY-SA 4.0 text](KiCad/CC-BY-SA-4.0.txt) are included here.
These terms apply to the library assets, not automatically to the board design.

License sources retrieved on the review date:

- [KiCad's licensing explanation](https://www.kicad.org/libraries/license/)
- [Upstream license, GitHub mirror revision b8b3cfdfad88ba66f21002b3de51dc6f7d55ba5a](https://github.com/KiCad/kicad-packages3D/blob/b8b3cfdfad88ba66f21002b3de51dc6f7d55ba5a/LICENSE.md)
- [Creative Commons legal text](https://creativecommons.org/licenses/by-sa/4.0/legalcode.txt)

The revision above identifies the license source, not the revision from which
each model was originally obtained. Per-file author credits remain in the STEP
headers.

## Teensy model

`Teensy_4.1_Assembly.STEP` is supplied by
[XenGi/teensy.pretty](https://github.com/XenGi/teensy.pretty/tree/126e054a2a02426f455ef2397ee5f6cb1ae80057),
whose README credits **Zack Kummer** for the Teensy 4.1 model. The repository's
MIT license, copyright (c) 2015 Ricardo Band, is retained verbatim in
[Teensy-LICENSE.txt](Teensy-LICENSE.txt). Preserve both the license and model
author attribution. This does not license the physical PJRC product or its
trademarks.

## Manufacturer/catalog models: publication not cleared

The product links in README.md are not proof of the exact STEP download source.
The STEP headers do not supply a redistribution license. In particular, several
headers use generic catalog filenames rather than manufacturer part numbers.
No model-specific permission overriding the restrictions below was found.

| File | Official evidence checked | Review result |
| --- | --- | --- |
| `NMJ6HCD2.step` | [Neutrik legal notice](https://www.neutrik.com/en/legal-notice) requires prior written consent for distribution beyond statutory permissions. | No public redistribution grant established. |
| `HRO_TYPE-C-31-M-12.step` | [HRO product page](https://www.krhro.com/Product-Details/726.html) states that its intellectual property must not be reproduced or used without consent; 3D data is available by request. | No public redistribution grant established. |
| `Hirose_DM3BT-DSF-PEJS.step` | [Hirose website terms, Article 1](https://www.hirose.com/product/accessAgreementPage/agreementPopup) prohibit reproduction of website works without permission. The terms were available through the search index; a direct fetch returned 404. | Exact download agreement and redistribution permission remain unresolved. |
| `SGTL5000_HVQFN32.step` | [NXP terms of use](https://www.nxp.com/pages/terms-of-use:TERMSOFUSE) limit the general content grant to unmodified personal noncommercial reproduction/distribution and otherwise require consent. | Not a sufficient grant for unrestricted public repository redistribution; exact catalog source also unverified. |
| `TLP2361.step` | [Toshiba STEP library](https://toshiba.semicon-storage.com/eu/semiconductor/design-development/eda-cad-lib/3d-step-library.html) and [European website terms](https://toshiba.semicon-storage.com/eu/terms.html): the general terms restrict copying to internal noncommercial purposes and prohibit distribution/public transmission. | No model-specific exception permitting repository distribution found. |

Before publishing these files, establish their exact source and obtain a license
permitting public redistribution, or replace them with independently created or
appropriately licensed models. Adding attribution or copying website terms does
not itself supply permission. No manufacturer license has been invented or
assigned here. Existing review renders also depict these models and should be
included in any permission or replacement review.

This review covers the new model bundle introduced by `d0c99ca`; it does not
certify the provenance of every pre-existing asset in the repository. DuPPa's
separate hardware and documentation notices remain in
[`../../i2cencoder_v2_1/NOTICE.md`](../../i2cencoder_v2_1/NOTICE.md).
