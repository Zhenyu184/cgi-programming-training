#ifndef _CGI_H_
#define _CGI_H_	1
#define CGI_LINE		1024
#define CGI_MAX_PATH		1024

#define CGI_Output_Html_Ex	CGI_Output_Html1

#define CGI_LOAD_HTML		"/home/httpd/cgi_load.html"

#define	UPLOAD_FILE_NAME	"file_name"
#define UPLOAD_FILE_SIZE	"file_size"
#define	UPLOAD_PERCENT		"percent"
#define UPLOAD_RESULT   	"result"
#define UPLOAD_PID      	"upload_pid"
#define UPLOAD_TMP      	"upload_tmp"
#define UPLOAD_MAX_FILENAME	127

#define CGI_FAIL			-1
#define CGI_SUCCESS			0
#define CGI_HTML_NOT_FOUND		-1
#define CGI_UPLOAD_LEN_NOT_ENOUGH	-2
#define CGI_UPLOAD_WRITE_ERROR		-3
#define CGI_UPLOAD_QUOTA_ERROR		-4
#define CGI_UPLOAD_OVER_SIZE_LIMIT_ERROR		-5
#define CGI_UPLOAD_NO_SUCH_FILE		-6
#define CGI_ENCRYPTION_SHARE_FOLDER_PATH_ALREADY_IN_USE -7
#define CGI_PERMISSION_DENIED -8


#define CGI_UNKNOWN		0
#define CGI_MSIE		1
#define CGI_NETSCAPE		2
#define CGI_MOZILLA_MAC		3
#define CGI_MOZILLA		4
#define CGI_MSIE_MAC		5
#define CGI_MOZILLA_LINUX	6
#define CGI_SAFARI		7

#define CGI_TYPE_UPLOAD		1
#define CGI_TYPE_NORMAL		2

#define CGI_REPLACE_OK		0
#define CGI_REPLACE_FAIL	-1

#define CGI_USE_DEFINE		1
#define CGI_USE_ORIGINAL	2

#define CGI_LANG_ENG		1
#define CGI_LANG_CHT		2
#define CGI_LANG_CHS		3
#define CGI_LANG_JPN		4
#define CGI_LANG_KOR		5
#define CGI_LANG_FRE		6
#define CGI_LANG_GER		7
#define CGI_LANG_ITA		8
#define CGI_LANG_POR		9
#define CGI_LANG_SPA		10
#define CGI_LANG_DUT		11
#define CGI_LANG_NOR		12
#define CGI_LANG_FIN		13
#define CGI_LANG_SWE		14
#define CGI_LANG_DAN		15
#define CGI_LANG_RUS		16
#define CGI_LANG_POL		17

// Bug#226946
#define CGI_UPLOAD_FILENAME_LENGTH 255

//coverity#81098
#define CGI_BOUNDARY_LENGTH 500

#define Copy_Str_Param(x, y)   snprintf(x, sizeof(x), "%s", y)

#define START_REOPEN_STD_012 \
	do { \
			int dev = open("/dev/null", 2); \
			if(dev != -1) \
			{ \
				close(0); \
				dup2(dev, 0); \
				close(1); \
				dup2(dev, 1); \
				close(2); \
				dup2(dev, 2); \
				close(dev); \
			} \
	} while(0);

#define END_REOPEN_STD_012	;

#define START_REOPEN_STD_12	 \
	do { \
		int dev = open("/dev/null", 2); \
		if(dev != -1) \
		{ \
			close(1); \
			dup2(dev, 1); \
			close(2); \
			dup2(dev, 2); \
			close(dev); \
		} \
	} while(0);

#define END_REOPEN_STD_12	;

#define START_REOPEN_STD_2	\
	do { \
		int dev = open("/dev/null", 2); \
		if(dev != -1) \
		{ \
			close(2); \
			dup2(dev, 2); \
			close(dev); \
		} \
	} while(0);

#define END_REOPEN_STD_2	;

enum {
	Q_ENG=0,
	Q_SCH,
	Q_TCH,
	Q_CZE,
	Q_DAN,
	Q_GER,
	Q_SPA,
	Q_FRE,
	Q_ITA,
	Q_JPN,
	Q_KOR,
	Q_NOR,
	Q_POL,
	Q_RUS,
	Q_FIN,
	Q_SWE,
	Q_DUT,
	Q_ESM,
	Q_TUR,
	Q_THA,
	Q_HUN,
	Q_POR,
	Q_GRK,
	Q_ROM
};	

#define B64_DECODE_EX(dest, src) b64_Decode_Ex(dest, sizeof(dest), src)
#define B64_ENCODE_EX(dest, src, src_len) b64_Encode_Ex(dest, sizeof(dest), src, src_len)

typedef struct qLANG_DEF {
	const char * u_lang;
	const char * l_lang;
	const char * note;
} qLANG_DEF;

typedef struct tagINPUT
{
        char 		*name;
        char 		*val;
        struct tagINPUT	*next;
} INPUT;

typedef struct
{
	char		pwd[64];
	char		host_addr[56];
	char		remote_user[256];
	char		remote_addr[56];
	long long	content_length;
	int		content_type;
	int		browser;
        int             cgi_category; // added by Jeff on 2012/12/18 for NVR fail-over, 0:NVR CFG 1:SYS CFG
        int             auth_result;  // added by Jeff on 2012/12/18 for NVR fail-over,
                                      //     0: SUCC non-taking-over mode 1: NVR SUCC, 2: SYS SUCC, 3 ALL SUCC
} HTTP_INFO;

typedef struct
{
	long long		predata;
	long long		nowdata;
	char		path[CGI_LINE];
	char		filename[CGI_LINE];
	char		src[CGI_MAX_PATH];
	int		type;
	int 		overwrite;
	char		mode[32];
	char		*progress;
	long long		sizelimit;
	long long		offset;
	int			wfm2Enable;
	char		*tmpfile;
	int		resume;
	int		cb; // for callback function
	void		*main_thread;
} UPLOAD;

typedef struct wfm_upload_file_t {
	int is_dest_exist;
	char owner[32];
	char group[32];
	char *upload_file_path;
} WFM_UPLOAD_FILE_INFO_T;

typedef void (*FuncUp_CB)();

// Archer Chang
#ifdef __cplusplus
extern "C" {
#endif

char	*strstrip(char *src, char *token, char **next);

#include <stdio.h>
/*QNAP fast cgi*/
#ifdef FCGI
#include "fcgiapp.h"
void 	QFCGI_Init(FCGX_Request* req);
int 	QFCGI_printf(const char *format, ...);
int     QFCGI_vprintf(const char *format, va_list arg);
char* 	QFCGI_getenv(char* env);
int 	QFCGI_putchar(int c);
int 	QFCGI_puts(const char *str);
#endif
/* below are export API function */
INPUT	*CGI_Get_Input();				/* need CGI_Free_Input */
INPUT	*CGI_Get_Input_URL();				/* need CGI_Free_Input */
INPUT	*CGI_Get_Input_FORM();				/* need CGI_Free_Input */
INPUT   *CGI_Get_Parameter(INPUT *arg, char *name);	/* need CGI_Free_Input */
INPUT   *CGI_Find_Parameter(INPUT *arg, char *name);
void	CGI_Free_Input(INPUT *arg);
void	CGI_Debug_Input(INPUT *arg);
int	CGI_Output_Html(char *filename, char *token, int (*replace)(FILE *fptr, char *oldstr));
int	CGI_Output_Html1(char *filename, char *token, int (*replace)(FILE *fptr, char *oldstr, void *), void *);
int     CGI_Output_Html_Init(char *end, char *filename);
int     CGI_Output_Html_End(char *start, char *filename, char *token, int (*replace1)(FILE *fptr, char *oldstr, void *arg1), void *arg1);
void	CGI_Get_Http_Info(HTTP_INFO *http_info);
void	CGI_Check_User();
int	CGI_Get_Language();
void	CGI_Init();
void	XML_Init();
int	CGI_Upload(char *path, char *filename, char *src);
int	CGI_Upload_Size_Limit(char *path, char *filename, char *src, long long sizelimit);
int     WFM2_CGI_Upload(char *path, char *filename, char *src, int overwrite, char *mode, char *progress);
int WFM2_CGI_Upload_for_Cb(char *path, char *filename, char *src, int overwrite, char *mode, char *progress, int cb, FuncUp_CB func_cb);
int WFM2_CGI_Upload_CB(char *path, char *filename, char *src, int overwrite, char *mode, char *progress, long long offset, int resume, FuncUp_CB func_cb);
int	CGI_Load_Html(char *filename);
int	CGI_Get_Text_IP(char *addr, int len, INPUT *input, char *name1, char *name2, char *name3, char *name4);
void 	Trim(char*);
char* URLdecode_to_UTF8(char* s);
char* URLdecode_to_UTF8_NotForm(char* s);
int b64_Decode_Ex(char *dest, int dest_size, const char *src);
int b64_Encode_Ex(char *dest, int dest_size, void *src, int src_len);
int Is_b64_Encode(char *str);
int Get_Tag_Value_By_CookieStrings(char *inputStr, char *tag, char *value, int value_len);
int Get_Cookie_Value_By_Tag(char *tag, char *value, int value_len);
int isMobileAgent(void);
int isTabletAgent(void);
int CGI_Get_Accept_Language(char *retStr, int retlen);
char* UTF8_to_URLencode(char* s);
int CGI_Handle_IE_Filename(char *dest_filename, char *src_filename);
int CGI_Handle_Filename(char *src_filename, char *out_filename, int outlen);
int CGI_Get_QLANG_Upper(int idx, char *retStr, int retlen);
int CGI_Get_QLANG_Lower(int idx, char *retStr, int retlen);
int CGI_Get_QLANG_Idx(int upper, char *langStr);

/* qsync enhancement functions */
int CGI_Get_Upload_ID(char *path, char *id, int size);
int CGI_Get_Tmp_Upload_ID(char *path, char *id, int size);
int WFM2_CGI_Upload_Ex(char *path, char *filename, char *src, int overwrite,
                            char *mode, char *progress, long long offset, int resume);

/* utility */
int Is_Valid_Number_Parameter(const char *param);

/* @newstr: Encoded string, in worse case, it need 6 times of space to origin */
void Trans_XML_Str(char *newstr, char *origin);
/* @newstr: Encoded string, in worse case, it need 4 times of space to origin */
void Trans_XML_Gt_Lt_Str(char *newstr, char *origin);

int	Get_Cookie_Ssid(int auth_type, char *ssid, int ssid_len);
int Is_CSRF_Vulnerability();

int Invalid_XML_Text(char *text);

#ifdef __cplusplus
}
#endif
#endif
