# `snprintf()` policy

XPDev public headers do not redefine the standard `snprintf()` or
`vsnprintf()` names. The standalone project requires a C11 compiler and uses
the implementation's conforming C library entry points directly.

This is an intentional difference from the historical Synchronet source-tree
behavior. `genwrap.h` previously mapped `snprintf()` to `safe_snprintf()` on
some platforms and to the pre-C99 `_snprintf()` on others. As a result, merely
including an XPDev header could change both the function called and its
truncation semantics. It also made identical source code behave differently on
Linux, BSD, macOS, and Windows.

## The explicitly named compatibility helper

`safe_snprintf()` remains available as an XPDev API for existing callers that
select it by name. It always terminates a non-empty destination and clamps a
successful truncating return value to `size - 1`. That return contract is not
the C99 `snprintf()` contract, which returns the number of characters that
would have been written.

The clamp is load-bearing for some historical callers that advance a buffer
pointer by the return value. Consequently, `safe_snprintf()` must not be
silently substituted for `snprintf()`, and changing its behavior requires a
separate compatibility review.

The installed-package consumer test verifies that XPDev headers define neither
standard name and that the platform `snprintf()` has its standard truncation
behavior.
