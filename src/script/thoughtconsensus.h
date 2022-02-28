// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2018-2022 Thought Network Ltd
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef THOUGHT_SCRIPT_THOUGHTCONSENSUS_H
#define THOUGHT_SCRIPT_THOUGHTCONSENSUS_H

#include <stdint.h>

#if defined(BUILD_THOUGHT_INTERNAL) && defined(HAVE_CONFIG_H)
#include <config/thought-config.h>
  #if defined(_WIN32)
    #if defined(DLL_EXPORT)
      #if defined(HAVE_FUNC_ATTRIBUTE_DLLEXPORT)
        #define EXPORT_SYMBOL __declspec(dllexport)
      #else
        #define EXPORT_SYMBOL
      #endif
    #endif
  #elif defined(HAVE_FUNC_ATTRIBUTE_VISIBILITY)
    #define EXPORT_SYMBOL __attribute__ ((visibility ("default")))
  #endif
#elif defined(MSC_VER) && !defined(STATIC_LIBTHOUGHTCONSENSUS)
  #define EXPORT_SYMBOL __declspec(dllimport)
#endif

#ifndef EXPORT_SYMBOL
  #define EXPORT_SYMBOL
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define THOUGHTCONSENSUS_API_VER 0

typedef enum thoughtconsensus_error_t
{
    thoughtconsensus_ERR_OK = 0,
    thoughtconsensus_ERR_TX_INDEX,
    thoughtconsensus_ERR_TX_SIZE_MISMATCH,
    thoughtconsensus_ERR_TX_DESERIALIZE,
    thoughtconsensus_ERR_INVALID_FLAGS,
} thoughtconsensus_error;

/** Script verification flags */
enum
{
    thoughtconsensus_SCRIPT_FLAGS_VERIFY_NONE                = 0,
    thoughtconsensus_SCRIPT_FLAGS_VERIFY_P2SH                = (1U << 0), // evaluate P2SH (BIP16) subscripts
    thoughtconsensus_SCRIPT_FLAGS_VERIFY_DERSIG              = (1U << 2), // enforce strict DER (BIP66) compliance
    thoughtconsensus_SCRIPT_FLAGS_VERIFY_NULLDUMMY           = (1U << 4), // enforce NULLDUMMY (BIP147)
    thoughtconsensus_SCRIPT_FLAGS_VERIFY_CHECKLOCKTIMEVERIFY = (1U << 9), // enable CHECKLOCKTIMEVERIFY (BIP65)
    thoughtconsensus_SCRIPT_FLAGS_VERIFY_CHECKSEQUENCEVERIFY = (1U << 10), // enable CHECKSEQUENCEVERIFY (BIP112)
    thoughtconsensus_SCRIPT_FLAGS_VERIFY_ALL                 = thoughtconsensus_SCRIPT_FLAGS_VERIFY_P2SH | thoughtconsensus_SCRIPT_FLAGS_VERIFY_DERSIG |
                                                            thoughtconsensus_SCRIPT_FLAGS_VERIFY_NULLDUMMY | thoughtconsensus_SCRIPT_FLAGS_VERIFY_CHECKLOCKTIMEVERIFY |
                                                            thoughtconsensus_SCRIPT_FLAGS_VERIFY_CHECKSEQUENCEVERIFY
};

/// Returns 1 if the input nIn of the serialized transaction pointed to by
/// txTo correctly spends the scriptPubKey pointed to by scriptPubKey under
/// the additional constraints specified by flags.
/// If not nullptr, err will contain an error/success code for the operation
EXPORT_SYMBOL int thoughtconsensus_verify_script(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen,
                                    const unsigned char *txTo        , unsigned int txToLen,
                                    unsigned int nIn, unsigned int flags, thoughtconsensus_error* err);

EXPORT_SYMBOL unsigned int thoughtconsensus_version();

#ifdef __cplusplus
} // extern "C"
#endif

#undef EXPORT_SYMBOL

#endif // THOUGHT_SCRIPT_DASHCONSENSUS_H
