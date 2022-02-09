#include <stdlib.h>
#include <stdio.h>

#include "pkcs11.h" // header file copied from tpm2-pkcs11 tag:1.6.0

#ifndef PERSISTENT_HANDLE
#error PERSISTENT_HANDLE is not defined
#endif

#ifndef AUTH_VALUE
#define AUTH_VALUE  "" // empty auth
#endif

#define ARRAY_LEN(x)         (sizeof(x)/sizeof(x[0]))
#define ADD_ATTR_BASE(t, x)  { .type = t,   .ulValueLen = sizeof(x),     .pValue = &x }
#define ADD_ATTR_ARRAY(t, x) { .type = t,   .ulValueLen = ARRAY_LEN(x),  .pValue = x }
#define ADD_ATTR_STR(t, x)   { .type = t,   .ulValueLen = sizeof(x) - 1, .pValue = x }

static const char *
CKR2Str(CK_ULONG res) {
	switch (res) {
	case CKR_OK:
		return "CKR_OK";
	case CKR_CANCEL:
		return "CKR_CANCEL";
	case CKR_HOST_MEMORY:
		return "CKR_HOST_MEMORY";
	case CKR_SLOT_ID_INVALID:
		return "CKR_SLOT_ID_INVALID";
	case CKR_GENERAL_ERROR:
		return "CKR_GENERAL_ERROR";
	case CKR_FUNCTION_FAILED:
		return "CKR_FUNCTION_FAILED";
	case CKR_ARGUMENTS_BAD:
		return "CKR_ARGUMENTS_BAD";
	case CKR_NO_EVENT:
		return "CKR_NO_EVENT";
	case CKR_NEED_TO_CREATE_THREADS:
		return "CKR_NEED_TO_CREATE_THREADS";
	case CKR_CANT_LOCK:
		return "CKR_CANT_LOCK";
	case CKR_ATTRIBUTE_READ_ONLY:
		return "CKR_ATTRIBUTE_READ_ONLY";
	case CKR_ATTRIBUTE_SENSITIVE:
		return "CKR_ATTRIBUTE_SENSITIVE";
	case CKR_ATTRIBUTE_TYPE_INVALID:
		return "CKR_ATTRIBUTE_TYPE_INVALID";
	case CKR_ATTRIBUTE_VALUE_INVALID:
		return "CKR_ATTRIBUTE_VALUE_INVALID";
	case CKR_DATA_INVALID:
		return "CKR_DATA_INVALID";
	case CKR_DATA_LEN_RANGE:
		return "CKR_DATA_LEN_RANGE";
	case CKR_DEVICE_ERROR:
		return "CKR_DEVICE_ERROR";
	case CKR_DEVICE_MEMORY:
		return "CKR_DEVICE_MEMORY";
	case CKR_DEVICE_REMOVED:
		return "CKR_DEVICE_REMOVED";
	case CKR_ENCRYPTED_DATA_INVALID:
		return "CKR_ENCRYPTED_DATA_INVALID";
	case CKR_ENCRYPTED_DATA_LEN_RANGE:
		return "CKR_ENCRYPTED_DATA_LEN_RANGE";
	case CKR_FUNCTION_CANCELED:
		return "CKR_FUNCTION_CANCELED";
	case CKR_FUNCTION_NOT_PARALLEL:
		return "CKR_FUNCTION_NOT_PARALLEL";
	case CKR_FUNCTION_NOT_SUPPORTED:
		return "CKR_FUNCTION_NOT_SUPPORTED";
	case CKR_KEY_HANDLE_INVALID:
		return "CKR_KEY_HANDLE_INVALID";
	case CKR_KEY_SIZE_RANGE:
		return "CKR_KEY_SIZE_RANGE";
	case CKR_KEY_TYPE_INCONSISTENT:
		return "CKR_KEY_TYPE_INCONSISTENT";
	case CKR_KEY_NOT_NEEDED:
		return "CKR_KEY_NOT_NEEDED";
	case CKR_KEY_CHANGED:
		return "CKR_KEY_CHANGED";
	case CKR_KEY_NEEDED:
		return "CKR_KEY_NEEDED";
	case CKR_KEY_INDIGESTIBLE:
		return "CKR_KEY_INDIGESTIBLE";
	case CKR_KEY_FUNCTION_NOT_PERMITTED:
		return "CKR_KEY_FUNCTION_NOT_PERMITTED";
	case CKR_KEY_NOT_WRAPPABLE:
		return "CKR_KEY_NOT_WRAPPABLE";
	case CKR_KEY_UNEXTRACTABLE:
		return "CKR_KEY_UNEXTRACTABLE";
	case CKR_MECHANISM_INVALID:
		return "CKR_MECHANISM_INVALID";
	case CKR_MECHANISM_PARAM_INVALID:
		return "CKR_MECHANISM_PARAM_INVALID";
	case CKR_OBJECT_HANDLE_INVALID:
		return "CKR_OBJECT_HANDLE_INVALID";
	case CKR_OPERATION_ACTIVE:
		return "CKR_OPERATION_ACTIVE";
	case CKR_OPERATION_NOT_INITIALIZED:
		return "CKR_OPERATION_NOT_INITIALIZED";
	case CKR_PIN_INCORRECT:
		return "CKR_PIN_INCORRECT";
	case CKR_PIN_INVALID:
		return "CKR_PIN_INVALID";
	case CKR_PIN_LEN_RANGE:
		return "CKR_PIN_LEN_RANGE";
	case CKR_PIN_EXPIRED:
		return "CKR_PIN_EXPIRED";
	case CKR_PIN_LOCKED:
		return "CKR_PIN_LOCKED";
	case CKR_SESSION_CLOSED:
		return "CKR_SESSION_CLOSED";
	case CKR_SESSION_COUNT:
		return "CKR_SESSION_COUNT";
	case CKR_SESSION_HANDLE_INVALID:
		return "CKR_SESSION_HANDLE_INVALID";
	case CKR_SESSION_PARALLEL_NOT_SUPPORTED:
		return "CKR_SESSION_PARALLEL_NOT_SUPPORTED";
	case CKR_SESSION_READ_ONLY:
		return "CKR_SESSION_READ_ONLY";
	case CKR_SESSION_EXISTS:
		return "CKR_SESSION_EXISTS";
	case CKR_SESSION_READ_ONLY_EXISTS:
		return "CKR_SESSION_READ_ONLY_EXISTS";
	case CKR_SESSION_READ_WRITE_SO_EXISTS:
		return "CKR_SESSION_READ_WRITE_SO_EXISTS";
	case CKR_SIGNATURE_INVALID:
		return "CKR_SIGNATURE_INVALID";
	case CKR_SIGNATURE_LEN_RANGE:
		return "CKR_SIGNATURE_LEN_RANGE";
	case CKR_TEMPLATE_INCOMPLETE:
		return "CKR_TEMPLATE_INCOMPLETE";
	case CKR_TEMPLATE_INCONSISTENT:
		return "CKR_TEMPLATE_INCONSISTENT";
	case CKR_TOKEN_NOT_PRESENT:
		return "CKR_TOKEN_NOT_PRESENT";
	case CKR_TOKEN_NOT_RECOGNIZED:
		return "CKR_TOKEN_NOT_RECOGNIZED";
	case CKR_TOKEN_WRITE_PROTECTED:
		return "CKR_TOKEN_WRITE_PROTECTED";
	case CKR_UNWRAPPING_KEY_HANDLE_INVALID:
		return "CKR_UNWRAPPING_KEY_HANDLE_INVALID";
	case CKR_UNWRAPPING_KEY_SIZE_RANGE:
		return "CKR_UNWRAPPING_KEY_SIZE_RANGE";
	case CKR_UNWRAPPING_KEY_TYPE_INCONSISTENT:
		return "CKR_UNWRAPPING_KEY_TYPE_INCONSISTENT";
	case CKR_USER_ALREADY_LOGGED_IN:
		return "CKR_USER_ALREADY_LOGGED_IN";
	case CKR_USER_NOT_LOGGED_IN:
		return "CKR_USER_NOT_LOGGED_IN";
	case CKR_USER_PIN_NOT_INITIALIZED:
		return "CKR_USER_PIN_NOT_INITIALIZED";
	case CKR_USER_TYPE_INVALID:
		return "CKR_USER_TYPE_INVALID";
	case CKR_USER_ANOTHER_ALREADY_LOGGED_IN:
		return "CKR_USER_ANOTHER_ALREADY_LOGGED_IN";
	case CKR_USER_TOO_MANY_TYPES:
		return "CKR_USER_TOO_MANY_TYPES";
	case CKR_WRAPPED_KEY_INVALID:
		return "CKR_WRAPPED_KEY_INVALID";
	case CKR_WRAPPED_KEY_LEN_RANGE:
		return "CKR_WRAPPED_KEY_LEN_RANGE";
	case CKR_WRAPPING_KEY_HANDLE_INVALID:
		return "CKR_WRAPPING_KEY_HANDLE_INVALID";
	case CKR_WRAPPING_KEY_SIZE_RANGE:
		return "CKR_WRAPPING_KEY_SIZE_RANGE";
	case CKR_WRAPPING_KEY_TYPE_INCONSISTENT:
		return "CKR_WRAPPING_KEY_TYPE_INCONSISTENT";
	case CKR_RANDOM_SEED_NOT_SUPPORTED:
		return "CKR_RANDOM_SEED_NOT_SUPPORTED";
	case CKR_RANDOM_NO_RNG:
		return "CKR_RANDOM_NO_RNG";
	case CKR_DOMAIN_PARAMS_INVALID:
		return "CKR_DOMAIN_PARAMS_INVALID";
	case CKR_BUFFER_TOO_SMALL:
		return "CKR_BUFFER_TOO_SMALL";
	case CKR_SAVED_STATE_INVALID:
		return "CKR_SAVED_STATE_INVALID";
	case CKR_INFORMATION_SENSITIVE:
		return "CKR_INFORMATION_SENSITIVE";
	case CKR_STATE_UNSAVEABLE:
		return "CKR_STATE_UNSAVEABLE";
	case CKR_CRYPTOKI_NOT_INITIALIZED:
		return "CKR_CRYPTOKI_NOT_INITIALIZED";
	case CKR_CRYPTOKI_ALREADY_INITIALIZED:
		return "CKR_CRYPTOKI_ALREADY_INITIALIZED";
	case CKR_MUTEX_BAD:
		return "CKR_MUTEX_BAD";
	case CKR_MUTEX_NOT_LOCKED:
		return "CKR_MUTEX_NOT_LOCKED";
	case CKR_VENDOR_DEFINED:
		return "CKR_VENDOR_DEFINED";
	}
	return "unknown PKCS11 error";
}

int
initialize(void)
{
    CK_RV rv = CKR_OK;
    CK_C_INITIALIZE_ARGS args = {
        .CreateMutex = NULL,
        .DestroyMutex = NULL,
        .LockMutex = NULL,
        .UnlockMutex = NULL,
        .flags = CKF_LIBRARY_CANT_CREATE_OS_THREADS // CKF_OS_LOCKING_OK, CKF_LIBRARY_CANT_CREATE_OS_THREADS
    };

    rv = C_Initialize(&args);
    if (rv != CKR_OK)
    {
        fprintf(stderr, "C_Initialize nok, error: %s\n", CKR2Str(rv));
        return 1;
    }

    fprintf(stdout, "C_Initialize ok\n");
    return 0;
}

int
finalize(void)
{
    CK_RV rv = CKR_OK;

    rv = C_Finalize(NULL_PTR);
    if (rv != CKR_OK)
    {
        fprintf(stderr, "C_Finalize nok, error: %s\n", CKR2Str(rv));
        return 1;
    }

    fprintf(stdout, "C_Finalize ok\n");
    return 0;
}

int
getInfo(void)
{
    CK_RV rv = CKR_OK;
    CK_INFO info;

    rv = C_GetInfo(&info);
    if (rv != CKR_OK)
    {
        fprintf(stderr, "C_GetInfo nok, error: %s\n", CKR2Str(rv));
        return 1;
    }

    fprintf(stdout, "C_GetInfo ok\n");
    return 0;
}

int
openSession(CK_SLOT_ID slot_id, CK_SESSION_HANDLE * session)
{
    CK_RV rv = CKR_OK;
    CK_FLAGS flags = CKF_SERIAL_SESSION | CKF_RW_SESSION;
    CK_SESSION_HANDLE s = CK_INVALID_HANDLE;

    rv = C_OpenSession(slot_id, flags, NULL, NULL, &s);
    if (rv != CKR_OK)
    {
        fprintf(stderr, "C_OpenSession nok, error: %s\n", CKR2Str(rv));
        return 1;
    }

    *session = s;

    fprintf(stdout, "C_OpenSession ok\n");
    //fprintf(stdout, "Session handle: 0x%lx\n", *session);
    return 0;
}

int
closeSession(CK_SESSION_HANDLE session)
{
    CK_RV rv = CKR_OK;

    rv = C_CloseSession(session);
    if (rv != CKR_OK)
    {
        fprintf(stderr, "C_CloseSession nok, error: %s\n", CKR2Str(rv));
        return 1;
    }

    fprintf(stdout, "C_CloseSession ok\n");
    return 0;
}

int
getSlot(CK_SLOT_ID * slot_id)
{
    CK_RV rv = CKR_OK;
    CK_BBOOL token_present = CK_TRUE;
    CK_ULONG i;
    CK_SLOT_ID slots[10];
    CK_ULONG slot_cnt = ARRAY_LEN(slots);

    rv = C_GetSlotList(token_present, slots, &slot_cnt);
    if (rv != CKR_OK)
    {
        fprintf(stderr, "C_GetSlotList nok, error: %s\n", CKR2Str(rv));
        return 1;
    }

    fprintf(stdout, "C_GetSlotList ok\n");

    *slot_id = 0;

    for (i=0; i < slot_cnt; i++) {
        CK_SLOT_ID s = slots[i];

        CK_TOKEN_INFO info = { 0 };
        rv = C_GetTokenInfo (s, &info);
        if (rv != CKR_OK)
        {
            fprintf(stderr, "C_GetTokenInfo nok, error: %s\n", CKR2Str(rv));
            return 1;
        }
        if (info.flags & CKF_TOKEN_INITIALIZED) {
            *slot_id = s;
            //fprintf(stdout, "Token id found: 0x%lx\n", *slot_id);
            //fprintf(stdout, "Token label: %s\n", info.label);
            //fprintf(stdout, "Token manufacturer id: %s\n", info.manufacturer_id);
            //fprintf(stdout, "Token model: %s\n", info.model);
            //fprintf(stdout, "Token serial number: %s\n", info.serial_number);
            break;
        }
    }

    if (i >= slot_cnt)
    {
        fprintf(stderr, "No initialized token found\n");
        return 1;
    }

    return 0;
}

int
login(CK_SESSION_HANDLE session)
{
#define USER_PIN "userpin"

    CK_RV rv = CKR_OK;
    CK_USER_TYPE user_type = CKU_USER;
    CK_UTF8CHAR pin[] = USER_PIN;

    rv = C_Login(session, user_type, pin, sizeof(pin) - 1);
    if (rv != CKR_OK)
    {
        fprintf(stderr, "C_Login nok, error: %s\n", CKR2Str(rv));
        return 1;
    }

    fprintf(stdout, "C_Login ok\n");

    return 0;
}

int
importKeyPairRsa(CK_SESSION_HANDLE session, CK_ULONG tpm_persistent_handle,
                 CK_CHAR_PTR auth_value)
{
    CK_BBOOL ck_true = CK_TRUE;
    CK_BBOOL ck_false = CK_FALSE;
    CK_BYTE id[] = {0x80, 0x81};
    CK_UTF8CHAR label[] = "tpm-rsa-key";
    CK_ULONG bits = 2048;
    CK_BYTE exp[] = { 0x00, 0x01, 0x00, 0x01 }; //65537 in BN

    CK_ATTRIBUTE pubAttr[] = {
        ADD_ATTR_BASE(CKA_TOKEN, ck_true),
        ADD_ATTR_BASE(CKA_PRIVATE, ck_false),
        ADD_ATTR_ARRAY(CKA_ID, id),
        ADD_ATTR_BASE(CKA_ENCRYPT, ck_true),
        ADD_ATTR_BASE(CKA_VERIFY, ck_true),
        ADD_ATTR_BASE(CKA_MODULUS_BITS, bits),
        ADD_ATTR_ARRAY(CKA_PUBLIC_EXPONENT, exp),
        ADD_ATTR_STR(CKA_LABEL, label)
    };

    CK_ATTRIBUTE privAttr[] = {
        ADD_ATTR_ARRAY(CKA_ID, id),
        ADD_ATTR_BASE(CKA_DECRYPT, ck_true),
        ADD_ATTR_BASE(CKA_SIGN, ck_true),
        ADD_ATTR_BASE(CKA_PRIVATE, ck_true),
        ADD_ATTR_BASE(CKA_TOKEN,   ck_true),
        ADD_ATTR_STR(CKA_LABEL, label),
        ADD_ATTR_BASE(CKA_SENSITIVE, ck_true)
    };

    CK_MECHANISM mech = {
        .mechanism = CKM_RSA_PKCS_KEY_PAIR_GEN,
        .pParameter = NULL,
        .ulParameterLen = 0
    };

    CK_RV rv = CKR_OK;

    rv = C_ImportKeyPair(session, &mech,
                           pubAttr, ARRAY_LEN(pubAttr),
                           privAttr, ARRAY_LEN(privAttr),
                           tpm_persistent_handle,
                           auth_value);
                           
    if (rv != CKR_OK)
    {
        fprintf(stderr, "C_ImportKeyPair nok, error: %s\n", CKR2Str(rv));
        return 1;
    }

    fprintf(stdout, "C_ImportKeyPair ok\n");

    return 0;
}

int
generateKeyPairRsa(CK_SESSION_HANDLE session)
{
    CK_BBOOL ck_true = CK_TRUE;
    CK_BBOOL ck_false = CK_FALSE;
    CK_BYTE id[] = {0x80, 0x81};
    CK_UTF8CHAR label[] = "tpm-rsa-key";
    CK_ULONG bits = 2048;
    CK_BYTE exp[] = { 0x00, 0x01, 0x00, 0x01 }; //65537 in BN

    CK_ATTRIBUTE pubAttr[] = {
        ADD_ATTR_BASE(CKA_TOKEN, ck_true),
        ADD_ATTR_BASE(CKA_PRIVATE, ck_false),
        ADD_ATTR_ARRAY(CKA_ID, id),
        ADD_ATTR_BASE(CKA_ENCRYPT, ck_true),
        ADD_ATTR_BASE(CKA_VERIFY, ck_true),
        ADD_ATTR_BASE(CKA_MODULUS_BITS, bits),
        ADD_ATTR_ARRAY(CKA_PUBLIC_EXPONENT, exp),
        ADD_ATTR_STR(CKA_LABEL, label)
    };

    CK_ATTRIBUTE privAttr[] = {
        ADD_ATTR_ARRAY(CKA_ID, id),
        ADD_ATTR_BASE(CKA_DECRYPT, ck_true),
        ADD_ATTR_BASE(CKA_SIGN, ck_true),
        ADD_ATTR_BASE(CKA_PRIVATE, ck_true),
        ADD_ATTR_BASE(CKA_TOKEN,   ck_true),
        ADD_ATTR_STR(CKA_LABEL, label),
        ADD_ATTR_BASE(CKA_SENSITIVE, ck_true)
    };

    CK_MECHANISM mech = {
        .mechanism = CKM_RSA_PKCS_KEY_PAIR_GEN,
        .pParameter = NULL,
        .ulParameterLen = 0
    };

    CK_RV rv = CKR_OK;
    CK_OBJECT_HANDLE pubkey = CK_INVALID_HANDLE;
    CK_OBJECT_HANDLE privkey = CK_INVALID_HANDLE;

    rv = C_GenerateKeyPair(session, &mech,
                           pubAttr, ARRAY_LEN(pubAttr),
                           privAttr, ARRAY_LEN(privAttr),
                           &pubkey, &privkey);
                           
    if (rv != CKR_OK)
    {
        fprintf(stderr, "C_GenerateKeyPair nok, error: %s\n", CKR2Str(rv));
        return 1;
    }

    fprintf(stdout, "C_GenerateKeyPair ok\n");

    return 0;
}

int
findObject(CK_SESSION_HANDLE session, CK_ATTRIBUTE_PTR attr, CK_ULONG attr_len, CK_OBJECT_HANDLE * handle)
{
    CK_RV rv = CKR_OK;
    CK_ULONG count = 0;

    rv = C_FindObjectsInit(session, attr, attr_len);
    if (rv != CKR_OK)
    {
        fprintf(stderr, "C_FindObjectsInit nok, error: %s\n", CKR2Str(rv));
        return 1;
    }

    rv = C_FindObjects(session, handle, 1, &count);
    if (rv != CKR_OK)
    {
        fprintf(stderr, "C_FindObjects nok, error: %s\n", CKR2Str(rv));
        return 1;
    }

    rv = C_FindObjectsFinal(session);
    if (rv != CKR_OK)
    {
        fprintf(stderr, "C_FindObjectsFinal nok, error: %s\n", CKR2Str(rv));
        return 1;
    }

    fprintf(stdout, "C_FindObjectsFinal ok\n");

    if (count == 0)
    {
        fprintf(stderr, "No Object found, exiting now\n");
        return 1;
    }

    return 0;
}

int
destroyKeyPairRsa(CK_SESSION_HANDLE session)
{
    CK_BBOOL ck_true = CK_TRUE;
    CK_BBOOL ck_false = CK_FALSE;
    CK_BYTE id[] = {0x80, 0x81};
    CK_UTF8CHAR label[] = "tpm-rsa-key";
    CK_ULONG bits = 2048;
    CK_BYTE exp[] = { 0x00, 0x01, 0x00, 0x01 }; //65537 in BN

    CK_ATTRIBUTE pubAttr[] = {
        ADD_ATTR_BASE(CKA_TOKEN, ck_true),
        ADD_ATTR_BASE(CKA_PRIVATE, ck_false),
        ADD_ATTR_ARRAY(CKA_ID, id),
        ADD_ATTR_BASE(CKA_ENCRYPT, ck_true),
        ADD_ATTR_BASE(CKA_VERIFY, ck_true),
        ADD_ATTR_BASE(CKA_MODULUS_BITS, bits),
        ADD_ATTR_ARRAY(CKA_PUBLIC_EXPONENT, exp),
        ADD_ATTR_STR(CKA_LABEL, label)
    };

    CK_ATTRIBUTE privAttr[] = {
        ADD_ATTR_ARRAY(CKA_ID, id),
        ADD_ATTR_BASE(CKA_DECRYPT, ck_true),
        ADD_ATTR_BASE(CKA_SIGN, ck_true),
        ADD_ATTR_BASE(CKA_PRIVATE, ck_true),
        ADD_ATTR_BASE(CKA_TOKEN,   ck_true),
        ADD_ATTR_STR(CKA_LABEL, label),
        ADD_ATTR_BASE(CKA_SENSITIVE, ck_true)
    };

    CK_RV rv = CKR_OK;
    CK_OBJECT_HANDLE pub = CK_INVALID_HANDLE;
    CK_OBJECT_HANDLE priv = CK_INVALID_HANDLE;

    if (findObject(session, pubAttr, ARRAY_LEN(pubAttr), &pub))
    {
        return 1;
    }

    rv = C_DestroyObject(session, pub);
    if (rv != CKR_OK)
    {
        fprintf(stderr, "C_DestroyObject nok, error: %s\n", CKR2Str(rv));
        return 1;
    }

    if (findObject(session, privAttr, ARRAY_LEN(privAttr), &priv))
    {
        return 1;
    }

    rv = C_DestroyObject(session, priv);
    if (rv != CKR_OK)
    {
        fprintf(stderr, "C_DestroyObject nok, error: %s\n", CKR2Str(rv));
        return 1;
    }

    fprintf(stdout, "C_DestroyKeyPair ok\n");

    return 0;
}

int main(int argc, char **argv)
{
    CK_SESSION_HANDLE session = CK_INVALID_HANDLE;
    CK_SLOT_ID slot_id = 0;
    CK_ULONG tpm_persistent_handle = PERSISTENT_HANDLE;
    CK_CHAR_PTR auth_value = (CK_CHAR_PTR)AUTH_VALUE;

    (void)tpm_persistent_handle;
    (void)auth_value;

    /** 
     * point it to an invalid path to prevent backend_esysdb from picking up any token
     * from the default dir ~/.tpm2_pkcs11/. Being overly cautious...
     * Expected side effect "ERROR: Cannot open database: unable to open database file"
     */
    //setenv("TPM2_PKCS11_STORE", "~/nil", 0);

    setenv("TPM2_PKCS11_BACKEND", "fapi", 0);
    setenv("TPM2_PKCS11_TCTI", "device:/dev/tpmrm0", 0);

    if (initialize() ||
        getInfo() ||
        getSlot(&slot_id) ||
        openSession(slot_id, &session) ||
        login(session) ||
        //generateKeyPairRsa(session) ||
        importKeyPairRsa(session, tpm_persistent_handle, auth_value) ||
        //destroyKeyPairRsa(session) ||
        closeSession(session) ||
        finalize())
    {
        return 1;
    }

    return 0;
}


