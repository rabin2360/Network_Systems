# if defined(__APPLE__)
# define COMMON_DIGEST_FOR_OPENSSL
# include <CommonCryto/CommonDigest.h>
# define SHA1 CC_SHA1
#else
# include <openssl/md5.h>
#endif

extern char *str2md5(const char * str, int length);
