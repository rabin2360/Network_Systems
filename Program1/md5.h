# if defined(__APPLE__)
# define COMMON_DIGEST_FOR_OPENSSL
# include <CommonCryto/CommonDigest.h>
# define SHA1 CC_SHA1
#else
# include <openssl/md5.h>
#endif

#define MAXBUFSIZE 30000

extern char *str2md5(const char * str, int length);
extern char **tokenize(char * str);
