#### 2022-01-11

* Do not make the device tree available since some OSes that support
  both ACPI and DT will prefer the latter by default.
* Enable auto clock gating.
* Fix Windows BSOD due to duplicate unique IDs on PRP0001 devices.
