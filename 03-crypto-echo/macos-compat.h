/**
 * macos-compat.h
 * Workaround for macOS encrypt() function name conflict
 */
#ifndef MACOS_COMPAT_H
#define MACOS_COMPAT_H

// On macOS, unistd.h declares encrypt() which conflicts with crypto-lib
// Rename our functions before including anything
#define encrypt crypto_lib_encrypt
#define decrypt crypto_lib_decrypt

#endif // MACOS_COMPAT_H