# Windows Mission Manager Placeholder Localization Design

## Goal

Make the Simplified Chinese translation of `Frame: %1` preserve its Qt
placeholder so `MissionManagerTest::_testErrorAckFailureStrings` produces the
same parameterized message as other locales.

## Context

On the approved Windows VS2022 x64 test environment, the fresh
`MissionManagerTest` run contains one failure and emits:

```text
QString::arg: Argument missing: "框架1", 3
```

`src/MissionManager/PlanManager.cc` constructs the diagnostic with
`tr("Frame: %1").arg(item->frame())`. The matching `zh_CN` translation is
`框架1`, which removes `%1`; Qt therefore cannot substitute the frame number.

## Options Considered

1. Change the `zh_CN` translation to `框架：%1` (chosen). This corrects the
   locale data at its source, retains the intended Chinese text, and preserves
   the production call site's parameter contract.
2. Change `PlanManager` to avoid the translated parameterized string. This
   would increase production-code scope and could leave the broken translation
   to affect future callers.
3. Force the test runner to use an English locale. This would hide a valid
   Simplified Chinese regression and would not repair user-visible output.

## Design

Modify only `translations/qgc_source_zh_CN.ts`, replacing the translation for
the existing source key `Frame: %1` with `框架：%1`. No source code, test
expectations, test whitelist, or locale environment setting will change.

The existing `MissionManagerTest` is the regression test: with the `zh_CN`
catalogue embedded in the Debug executable, it must no longer emit the Qt
missing-argument warning and must pass `_testErrorAckFailureStrings`.

## Validation

Use the established VS2022 v143 x64, Qt 6.10.3 `msvc2022_64`, Ninja, and
GStreamer environment. Rebuild the affected Debug target without manual
`CMAKE_CXX_FLAGS`, then run only `MissionManagerTest` with JUnit and console
logging enabled:

```powershell
$env:QGC_TEST_VERBOSE = '1'
& .\build\windows-debug-vs2022-zc-fixed\Debug\MissionManagerTest.exe `
  --unittest-output:.tmp\phase2d\MissionManagerTest.xml `
  --log-output `
  --logging:default
```

Success requires JUnit to report 7 tests with 0 failures, 0 errors, and 0
skips, and the console log must contain no `QString::arg: Argument missing`
entry. This phase does not run full CTest and does not alter the separate
runner investigations for `HashCheckTest`, `SigningTest`, or
`RequestMetaDataTypeStateMachineTest`.

## Scope Boundaries

- Windows Desktop Debug verification only.
- Preserve the existing translation file format and catalogue source key.
- Do not modify business source code or test runners.
- Do not force an English locale, suppress Qt diagnostics, or whitelist the
  warning.
