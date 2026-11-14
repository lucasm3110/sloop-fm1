Lucas FM 0.2 evidence for source 0bce425064ef00b5b3bc711c6b4f14a0d47970d9.

*.metrics.json contain exact actual-firmware measurements; stress cases include input scripts and artifact hashes. audio-checks.txt checks the complete preloaded package without an external flash or sample overlay. Startup has one initial late counter in these strict bench cases; there are no additional late counters during the tested playback. emulator-summary.txt includes screen, save/reload and stress cases. voice7-test.txt covers maximum heavy engine load with visible effect protection.

upstream.txt is the unedited full CI output, including four static budget failures and the old site-generator identity guard. The latter was fixed after the firmware build; site-tests.txt verifies the corrected generator on the exact 0.2 package. The full CI was not rerun for documentation/test-tool changes. No golden or instruction-budget baselines changed.

Personal audio is excluded. Hardware remains untested.
