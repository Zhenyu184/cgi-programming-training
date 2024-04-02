//==========================================================================
//
//	Copyright (c) 2000  ICP Electronics Inc.  All Rights Reserved.
//
//	FILE:
//		cgi.c
//
//	Abstract:
//		cgi library
//
//	FUNCTIONS:
//
//	COMMENTS:	N/A
//
//	HISTORY:
//		2001/07/15	Kent create
//		2001/08/09	Add CGI_Upload() function
//		2001/09/10	Add v2_menu functions
//		2001/09/26	Add v2_menu error function
//
//==========================================================================
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <regex.h>
#include <libgen.h>
#include <signal.h>
#include <limits.h>

/* NAS lib include */
#include <cgi.h>
#include <NAS.h>
#include <config.h>
#include <Util.h>
#include <auth_session.h>

#include <debug.h>
#ifdef RECYCLE_EX
#include <asm/unistd.h>
#include <linux/unistd.h>   //for qnap_unlink system call
#define qnap_unlink(a) syscall(__NR_qnap_unlink, a)
#endif

//#define _DEBUG	1
#define _FILE_OFFSET_BITS 64
pthread_mutex_t sleep_mutex;
pthread_cond_t sleep_cond;

static char* hex[256] = {
    "%00", "%01", "%02", "%03", "%04", "%05", "%06", "%07",
    "%08", "%09", "%0a", "%0b", "%0c", "%0d", "%0e", "%0f",
    "%10", "%11", "%12", "%13", "%14", "%15", "%16", "%17",
    "%18", "%19", "%1a", "%1b", "%1c", "%1d", "%1e", "%1f",
    "%20", "%21", "%22", "%23", "%24", "%25", "%26", "%27",
    "%28", "%29", "%2a", "%2b", "%2c", "%2d", "%2e", "%2f",
    "%30", "%31", "%32", "%33", "%34", "%35", "%36", "%37",
    "%38", "%39", "%3a", "%3b", "%3c", "%3d", "%3e", "%3f",
    "%40", "%41", "%42", "%43", "%44", "%45", "%46", "%47",
    "%48", "%49", "%4a", "%4b", "%4c", "%4d", "%4e", "%4f",
    "%50", "%51", "%52", "%53", "%54", "%55", "%56", "%57",
    "%58", "%59", "%5a", "%5b", "%5c", "%5d", "%5e", "%5f",
    "%60", "%61", "%62", "%63", "%64", "%65", "%66", "%67",
    "%68", "%69", "%6a", "%6b", "%6c", "%6d", "%6e", "%6f",
    "%70", "%71", "%72", "%73", "%74", "%75", "%76", "%77",
    "%78", "%79", "%7a", "%7b", "%7c", "%7d", "%7e", "%7f",
    "%80", "%81", "%82", "%83", "%84", "%85", "%86", "%87",
    "%88", "%89", "%8a", "%8b", "%8c", "%8d", "%8e", "%8f",
    "%90", "%91", "%92", "%93", "%94", "%95", "%96", "%97",
    "%98", "%99", "%9a", "%9b", "%9c", "%9d", "%9e", "%9f",
    "%a0", "%a1", "%a2", "%a3", "%a4", "%a5", "%a6", "%a7",
    "%a8", "%a9", "%aa", "%ab", "%ac", "%ad", "%ae", "%af",
    "%b0", "%b1", "%b2", "%b3", "%b4", "%b5", "%b6", "%b7",
    "%b8", "%b9", "%ba", "%bb", "%bc", "%bd", "%be", "%bf",
    "%c0", "%c1", "%c2", "%c3", "%c4", "%c5", "%c6", "%c7",
    "%c8", "%c9", "%ca", "%cb", "%cc", "%cd", "%ce", "%cf",
    "%d0", "%d1", "%d2", "%d3", "%d4", "%d5", "%d6", "%d7",
    "%d8", "%d9", "%da", "%db", "%dc", "%dd", "%de", "%df",
    "%e0", "%e1", "%e2", "%e3", "%e4", "%e5", "%e6", "%e7",
    "%e8", "%e9", "%ea", "%eb", "%ec", "%ed", "%ee", "%ef",
    "%f0", "%f1", "%f2", "%f3", "%f4", "%f5", "%f6", "%f7",
    "%f8", "%f9", "%fa", "%fb", "%fc", "%fd", "%fe", "%ff"
};

const qLANG_DEF qLANG_ARRAY[] = {
  { "ENG","eng",NULL },
  { "SCH","chs",NULL },
  { "TCH","cht",NULL },
  { "CZE","cze",NULL },
  { "DAN","dan",NULL },
  { "GER","ger",NULL },
  { "SPA","spa",NULL },
  { "FRE","fre",NULL },
  { "ITA","ita",NULL },
  { "JPN","jpn",NULL },
  { "KOR","kor",NULL },
  { "NOR","nor",NULL },
  { "POL","pol",NULL },
  { "RUS","rus",NULL },
  { "FIN","fin",NULL },
  { "SWE","swe",NULL },
  { "DUT","dut",NULL },
  { "ESM","es-mex",NULL },
  { "TUR","tur",NULL },
  { "THA","tha",NULL },
  { "HUN","hun",NULL },
  { "POR","por",NULL },
  { "GRK","grk",NULL },
  { "ROM","rom",NULL },
  {NULL, NULL, NULL}
};

char *ReadStdin(FILE *f, char stop, int *len)
{
	size_t wsize;
	char *word, *new_word = NULL;
	int x;

	wsize = 128;
	x = 0;
	word = (char *) malloc(sizeof(char)*(wsize+1));
	if (word==NULL)
		return NULL;
	while (1)
	{
		word[x] = (char)fgetc(f);
		if (x==wsize)
		{
			wsize+=128;
			new_word = (char *) realloc(word, sizeof(char)*(wsize+1));
                        if(new_word == NULL){
                                word[x] = '\0';
                                return word;
                        }
                        word = new_word;
			word[x+1] = '\0';
		}
		--(*len);
		if ((word[x]==stop) || (feof(f)) || (!(*len)) || (x == INT_MAX-1))
		{
			if (word[x]!=stop)
				x++;
			word[x] ='\0';
			return word;
		}
		++x;
	}
}

char *ReadData(char *line, char stop)
{
	int i=0, j;
	char *word = (char *) malloc(sizeof(char)*(strlen(line)+1));

	if (word==NULL)
		return NULL;
	for (i =0;((line[i]) && (line[i]!=stop));i++)
		word[i]=line[i];
	word[i]='\0';
	if (line[i])
		++i;
	j=0;
	while (line[j])
		line[j++]=line[i++];
	return word;
}

char trans_http_char(char *pattern)
{
	char ret;
	long asciicode;

	asciicode=strtol(pattern, NULL, 16);
	ret=(char)asciicode;
//Modify by Richard Chang 20080416 for X86 CPU 
#if defined (I386) || defined (PPC)
	if (	(ret>=' ' && ret<='/') || ((int)ret>=':' && (int)ret<='@') || ret<0 ||
		(ret>='[' && ret<='`') || (ret>='{' && ret<='~'))
#elif defined(ARM)
//Modify by Ken Chen 20050924 for ARM CPU
	if (	(ret>=' ' && ret<='/') || ((int)ret>=':' && (int)ret<='@') || ret>=128 ||
		(ret>='[' && ret<='`') || (ret>='{' && ret<='~'))
#endif
		return ret;
	else
		return 0x0;
}

void trans_http_str(char *new, char *origin, int bform)
{
	char *ptr, *start;
	char pattern[3];

	ptr=origin;
	start=new;
	
	while (*ptr!=0x0)
	{
		if (*ptr=='%')
		{
			pattern[0]=ptr[1];
			pattern[1]=ptr[2];
			pattern[2]=0x0;
			*start=trans_http_char(pattern);
			if (*start==0x0)
			{
				*start++='%';
				ptr++;
			}
			else
			{
				start++;
				ptr+=3;
			}
		}
		else
		if (*ptr=='+')
		{
			if (bform)
			{
				ptr++;
				*start++=' ';
			}
			else
				*start++=*ptr++;
		}
		else
		{
			*start++=*ptr++;
		}
	}
	*start=0x0;
}

/*
 *  Desc: Escape XML special character
 *  @Param newstr: Encoded string, in worse case, it need 6 times of space to origin
 *  @Param origin: Origin string
 */
void Trans_XML_Str(char *newstr, char *origin)
{
	char *ptr, *start;

	ptr=origin;
	start=newstr;
	while (*ptr!=0)
	{
		if (*ptr=='\'')
		{
			*start++='&';
			*start++='a';
			*start++='p';
			*start++='o';
			*start++='s';
			*start++=';';
			ptr++;
		}
		else
		if (*ptr=='"')
		{
			*start++='&';
			*start++='q';
			*start++='u';
			*start++='o';
			*start++='t';
			*start++=';';
			ptr++;
		}
		else
		if (*ptr=='>')
		{
			*start++='&';
			*start++='g';
			*start++='t';
			*start++=';';
			ptr++;
		}
		else
		if (*ptr=='<')
		{
			*start++='&';
			*start++='l';
			*start++='t';
			*start++=';';
			ptr++;
		}
		else
		if (*ptr=='&')
		{
			*start++='&';
			*start++='a';
			*start++='m';
			*start++='p';
			*start++=';';
			ptr++;
		}
		else
			*start++=*ptr++;
	}
	*start=0x0;
}

/*
 *  Desc: Escape '<' and '>' character
 *  @Param newstr: Encoded string, in worse case, it need 4 times of space to origin
 *  @Param origin: Origin string
 */
void Trans_XML_Gt_Lt_Str(char *newstr, char *origin)
{
	char *ptr, *start;

	ptr=origin;
	start=newstr;
	while (*ptr!=0)
	{
		if (*ptr=='>')
		{
			*start++='&';
			*start++='g';
			*start++='t';
			*start++=';';
			ptr++;
		}
		else
		if (*ptr=='<')
		{
			*start++='&';
			*start++='l';
			*start++='t';
			*start++=';';
			ptr++;
		}
		else
			*start++=*ptr++;
	}
	*start=0x0;
}

/* The fiffecence to URLdecode_to_UTF8 API: not handle '+' character */
char* URLdecode_to_UTF8_NotForm(char* s)
{
	char *sbuf;
	int l  = strlen(s);
	int ch = -1 ;

	int i, b, num = 0, more;

	sbuf = calloc( 1, l+1 );

	for (i = 0, more = -1 ; i < l ; i++) {
		int hb, lb;

		switch (ch = s[i]) {
			case '%':
				ch = s[++i];
	          		hb = (isdigit ((char) ch) ? ch - '0' : 10+tolower((char) ch) - 'a') & 0xF ;
        	  		ch = s[++i];
          			lb = (isdigit ((char) ch) ? ch - '0' : 10+tolower((char) ch) - 'a') & 0xF ;
          			b = (hb << 4) | lb ;
	          		break ;
// HugoLiao: fix 48011
#ifdef _VIOSTOR_
			case '+':
				b = ' ';
				break;
#endif
        		default:
          			b = ch ;
    		}
		sbuf[num++] = (char)b;
	}
	return sbuf;
}

//============================================================
//	this function is parser the parameter of cgi
//	the separate char is "&"
//	ex: xxx.cgi?name1=value1&name2=value2&name3=value3
//	result: arg[0].name=name1, arg[0].val=value1
//		arg[1].name=name2, arg[1].val=value2
//		arg[2].name=name3, arg[2].val=value3
//============================================================
INPUT *CGI_Get_Input_URL()
{
	INPUT		*arg=NULL, *start=NULL;
	int		count=0;
	char	*token=NULL, *ptr=NULL, *utf8_ret1=NULL, *utf8_ret2=NULL;
	char	tokensep[]="&";

	ptr=(char *)getenv("QUERY_STRING");
	if (ptr==NULL)
		return NULL;
	token=strtok(ptr, tokensep);
	while (token!=NULL)
	{
		if (count==0)
		{
			if ((arg=calloc(1, sizeof(INPUT)))==NULL)
				return NULL;
			start=arg;
			count++;
		}
		else
		{
			if ((arg->next=calloc(1, sizeof(INPUT)))==NULL)
				goto error_get_input_URL;
			arg=(INPUT *)arg->next;
		}
		ptr=token;
		token=strstr(token,"=");
		if (token==NULL)
			return start;
		*token=0x0;
		if ((arg->name=calloc(1, strlen(ptr)+1))==NULL)
			goto error_get_input_URL;
		strcpy(arg->name, ptr);
		token++;
		if ((arg->val=calloc(1, strlen(token)+1))==NULL)
			goto error_get_input_URL;
		strcpy(arg->val, token);
//		trans_http_str(arg->val, arg->val, 0);
		token=strtok(NULL, tokensep);
                utf8_ret1 = URLdecode_to_UTF8_NotForm(arg->name);
                utf8_ret2 = URLdecode_to_UTF8_NotForm(arg->val);
                if (utf8_ret1) {
                    strcpy(arg->name, utf8_ret1);
                    free(utf8_ret1);
                }
                if (utf8_ret2) {
                    strcpy(arg->val, utf8_ret2);
                    free(utf8_ret2);
                }
	}
	return start;
error_get_input_URL:
	CGI_Free_Input(start);
	return NULL;
}

//add by Ken Chen 20050922
//fix by Ken Chen 20070201
char* URLdecode_to_UTF8(char* s)
{
    //char sbuf[2048] = {0};
    char *sbuf;
    int l  = strlen(s);
    int ch = -1 ;
//    int i, b, sumb = 0, num = 0, more;
    int i, b, num = 0, more;
    
    sbuf = calloc( 1, l+1 );
    
    for (i = 0, more = -1 ; i < l ; i++) {
      /* Get next byte b from URL segment s */
    int hb, lb;
    switch (ch = s[i]) {   	
	case '%':
	  ch = s[++i];
	  hb = (isdigit ((char) ch) ? ch - '0' : 10+tolower((char) ch) - 'a') & 0xF ;
	  ch = s[++i];
	  lb = (isdigit ((char) ch) ? ch - '0' : 10+tolower((char) ch) - 'a') & 0xF ;
	  b = (hb << 4) | lb ;
	  break ;
	case '+':
	  b = ' ' ;
	  break ;
	default:
	  b = ch ;
    }
    sbuf[num++] = (char)b;

#if 0		
      /* Decode byte b as UTF-8, sumb collects incomplete chars */
      if ((b & 0xc0) == 0x80) {			// 10xxxxxx (continuation byte)
				sumb = (sumb << 6) | (b & 0x3f) ;	// Add 6 bits to sumb
				if (--more == 0)  
				{
					//sbuf[num++] = (char)sumb; //sbuf.append((char) sumb) ; // Add char to sbuf
					sbuf[num++] = (char)(sumb & 0xFF);
					sbuf[num++] = (char)((sumb & 0xFF00) >> 8);
				}
      } else if ((b & 0x80) == 0x00) {		// 0xxxxxxx (yields 7 bits)
				//sbuf.append((char) b) ;			// Store in sbuf
				sbuf[num++] = (char)b;
      } else if ((b & 0xe0) == 0xc0) {		// 110xxxxx (yields 5 bits)
				sumb = b & 0x1f;
				more = 1;				// Expect 1 more byte
      } else if ((b & 0xf0) == 0xe0) {		// 1110xxxx (yields 4 bits)
				sumb = b & 0x0f;
				more = 2;				// Expect 2 more bytes
      } else if ((b & 0xf8) == 0xf0) {		// 11110xxx (yields 3 bits)
				sumb = b & 0x07;
				more = 3;				// Expect 3 more bytes
      } else if ((b & 0xfc) == 0xf8) {		// 111110xx (yields 2 bits)
				sumb = b & 0x03;
				more = 4;				// Expect 4 more bytes
      } else /*if ((b & 0xfe) == 0xfc)*/ {	// 1111110x (yields 1 bit)
				sumb = b & 0x01;
				more = 5;				// Expect 5 more bytes
      }
#endif
      /* No need to test if the UTF-8 encoding is well-formed */
    }
    //return strdup(sbuf);
    return sbuf;
}
//end here

INPUT *CGI_Get_Input_FORM()
{
	INPUT	*form=NULL, *start=NULL;
	char	*ptr=NULL, *utf8_ret1=NULL, *utf8_ret2=NULL;
	int	len=0, i=0, count=0, input_len=0;

	ptr=(char *)getenv("CONTENT_TYPE");
	if (ptr!=NULL)
	{
		if (strstr(ptr, "multipart/form-data"))
			return NULL;
	}

	if ((ptr=getenv("CONTENT_LENGTH"))==NULL)
		return NULL;
	len=atoi(ptr);
	count=0;
	for (i=0;len && !feof(stdin);i++)	   // Read data from stdin
	{
		if (count==0)
		{
			if ((form=calloc(1, sizeof(INPUT)))==NULL)
				goto error_get_input_form;
			start=form;
			count++;
		}
		else
		{
			if ((form->next=calloc(1, sizeof(INPUT)))==NULL)
				goto error_get_input_form;
			form=(INPUT *)form->next;
		}
		if ((form->val = ReadStdin(stdin, '&', &len))==NULL)
			goto error_get_input_form;
		if ((form->name = ReadData(form->val,'='))==NULL)
			goto error_get_input_form;

		/* for type=image used */
		input_len=strlen(form->name);
		if (form->name[input_len-1]=='y' && form->name[input_len-2]=='.')
		{
			int	len1, len2;
			INPUT	*tmp;

			tmp=form;
			len1=strlen(tmp->val);
			len2=strlen(tmp->name);
			if ((form->next=calloc(1, sizeof(INPUT)))==NULL)
				goto error_get_input_form;
			form=(INPUT *)form->next;
			form->val=calloc(1, len1+1);
			form->name=calloc(1, len2+1);
			strcpy(form->val, tmp->val);
			strcpy(form->name, tmp->name);
			form->name[len2-2]=0x0;
		}
		//add by Ken Chen 20050922
		utf8_ret1 = URLdecode_to_UTF8(form->name);
		utf8_ret2 = URLdecode_to_UTF8(form->val);
		if (utf8_ret1) {
            strcpy(form->name, utf8_ret1);
            free(utf8_ret1);
        }
        if (utf8_ret2) {
            strcpy(form->val, utf8_ret2);
            free(utf8_ret2);
        }
		//end here
		//Modified by KenChen 20060305 - not to transfer '+' to ' '
// Kent 2009-07-16, needn't transfer form data
// for example: input data "%Aaxxx" => will be transfered to a incorrect data
//		trans_http_str(form->val, form->val, 0);
//		trans_http_str(form->name, form->name, 0);
		// end here
	}
	return start;
error_get_input_form:
	CGI_Free_Input(start);
	return NULL;
}

INPUT *CGI_Get_Input()
{
	INPUT	*url=NULL, *form=NULL, *input=NULL, *ptr=NULL;

	url=CGI_Get_Input_URL();
	form=CGI_Get_Input_FORM();
	if (url==NULL)
		return form;
	input=url;
	ptr=url;
	while (ptr!=NULL)
	{
		if (ptr->next==NULL)
		{
			ptr->next=form;
			break;
		}
		ptr=(INPUT *)ptr->next;
	}
	return input;
}

void CGI_Free_Input(INPUT *arg)
{
	INPUT *tmp;

	tmp=arg;
	while (arg!=NULL)
	{
		tmp=(INPUT *)arg->next;
		if (arg->name!=NULL)
			free(arg->name);
		if (arg->val!=NULL)
			free(arg->val);
		free(arg);
		arg=tmp;
	}
}

void CGI_Debug_Input(INPUT *arg)
{
	int i=0;

	while (arg!=NULL)
	{
		printf("arg[%d].name=%s<br>\n", i, arg->name);
		printf("arg[%d].val=%s<br>\n", i, arg->val);
		arg=(INPUT *)arg->next;
		i++;
	}
}

int CGI_Get_Language()
{
	char	*ptr;
	int	ret=CGI_LANG_ENG;

	ptr=(char*)getenv("HTTP_COOKIE");
	if (ptr!=NULL)
	{
		if (strstr(ptr, "nas_lang=ENG"))
			ret=CGI_LANG_ENG;
		else
		if (strstr(ptr, "nas_lang=TCH"))
			ret=CGI_LANG_CHT;
		else
		if (strstr(ptr, "nas_lang=SCH"))
			ret=CGI_LANG_CHS;
		else
		if (strstr(ptr, "nas_lang=JPN"))
			ret=CGI_LANG_JPN;
		else
		if (strstr(ptr, "nas_lang=KOR"))
			ret=CGI_LANG_KOR;
		else
		if (strstr(ptr, "nas_lang=FRE"))
			ret=CGI_LANG_FRE;
		else
		if (strstr(ptr, "nas_lang=GER"))
			ret=CGI_LANG_GER;
		else
		if (strstr(ptr, "nas_lang=ITA"))
			ret=CGI_LANG_ITA;
		else
		if (strstr(ptr, "nas_lang=POR"))
			ret=CGI_LANG_POR;
		else
		if (strstr(ptr, "nas_lang=SPA"))
			ret=CGI_LANG_SPA;
		else
		if (strstr(ptr, "nas_lang=DUT"))
			ret=CGI_LANG_DUT;
		else
		if (strstr(ptr, "nas_lang=NOR"))
			ret=CGI_LANG_NOR;
		else
		if (strstr(ptr, "nas_lang=FIN"))
			ret=CGI_LANG_FIN;
		else
		if (strstr(ptr, "nas_lang=SWE"))
			ret=CGI_LANG_SWE;
		else
		if (strstr(ptr, "nas_lang=DAN"))
			ret=CGI_LANG_DAN;
		else
		if (strstr(ptr, "nas_lang=RUS"))
			ret=CGI_LANG_RUS;
		else
		if (strstr(ptr, "nas_lang=POL"))
			ret=CGI_LANG_POL;
	}
	return ret;
}

int global_replace(FILE *fptr, char *name)
{
	int	counter;

	if (!strcasecmp(name, "counter"))
	{
		counter=time(NULL);
		fprintf(fptr, "%d", counter);
	}
	else
	if (!strcasecmp(name, "lang") || !strcasecmp(name, "charset"))
	{
			fprintf(fptr, "UTF-8");
	}
	else
	if (!strcmp(name, "SERVER_NAME"))
	{
		char	buf[80];

		Get_Server_Name(buf, 80);
		fprintf(fptr, "%s", buf);
	}
	else
		return CGI_REPLACE_FAIL;
	return CGI_REPLACE_OK;
}

char *strstrip(char *src, char *token, char **next)
{
	char *head, *tail;

	head=src;
	while (*head!=0x0)
	{
		if (*head==*token)
		{
			if (!strncmp(head, token, strlen(token)))
			{
				tail=head+strlen(token);
				while (*tail!=0x0)
				{
					if (*tail==*token)
					{
						if (!strncmp(tail, token, strlen(token)))
						{
							*head=0x0;
							*tail=0x0;
							*next=tail+strlen(token);
							return head+strlen(token);
						}
					}
					tail++;
				}
			}
		}
		head++;
	}
	return NULL;
}

int CGI_Output_Html(char *filename, char *token, int (*replace)(FILE *fptr, char *oldstr))
{
	FILE	*fptr;
//	char	line[CGI_LINE], output[CGI_LINE], *tmpstr, newstr[CGI_LINE];
	char line[CGI_LINE];
	int	ret=CGI_REPLACE_FAIL;
	char	*ptr, *next, *val;

	if ((fptr=fopen(filename, "r"))==NULL)
		return CGI_HTML_NOT_FOUND;
	while (!feof(fptr))
	{
		if (fgets(line, CGI_LINE, fptr)!=NULL)
		{
			ptr=line;
			while ((val=strstrip(ptr, token, &next))!=NULL)
			{
				fprintf(stdout, "%s", ptr);
				ret=CGI_REPLACE_FAIL;
				if (replace!=NULL)
					ret=replace(stdout, val);
				if (ret==CGI_REPLACE_FAIL)
					ret=global_replace(stdout, val);
				if (ret==CGI_REPLACE_FAIL)
					fprintf(stdout, "%s%s%s", token, val, token);
				ptr=next;
			}
			fprintf(stdout, "%s", ptr);
		}
	}
	fclose(fptr);
	return CGI_SUCCESS;
}

int CGI_Output_Html1(char *filename, char *token, int (*replace1)(FILE *fptr, char *oldstr, void *arg1), void *arg1)
{
	FILE	*fptr;
//	char	line[CGI_LINE], output[CGI_LINE], *tmpstr, newstr[CGI_LINE];
	char    line[CGI_LINE];
	int	ret=CGI_REPLACE_FAIL;
	char	*ptr, *next, *val;

	if ((fptr=fopen(filename, "r"))==NULL)
		return CGI_HTML_NOT_FOUND;
	while (!feof(fptr))
	{
		if (fgets(line, CGI_LINE, fptr)!=NULL)
		{
			ptr=line;
			while ((val=strstrip(ptr, token, &next))!=NULL)
			{
				fprintf(stdout, "%s", ptr);
				ret=CGI_REPLACE_FAIL;
				if (replace1!=NULL)
					ret=replace1(stdout, val, arg1);
				if (ret==CGI_REPLACE_FAIL)
					ret=global_replace(stdout, val);
				if (ret==CGI_REPLACE_FAIL)
					fprintf(stdout, "%s%s%s", token, val, token);
				ptr=next;
			}
			fprintf(stdout, "%s", ptr);
		}
	}
	fclose(fptr);
	return CGI_SUCCESS;
}

int CGI_Output_Html_Init(char *end, char *filename)
{
        FILE	*fptr;
        char    line[CGI_LINE];
        char	*ptr;

        if ((fptr=fopen(filename, "r"))==NULL)
                return CGI_HTML_NOT_FOUND;

        while (!feof(fptr))
        {
                if (fgets(line, CGI_LINE, fptr)!=NULL)
                {
                        if(strstr(line, end))
                                break;
                        ptr = line;
                        fprintf(stdout, "%s", ptr);
                }
        }
        fclose(fptr);
        return CGI_SUCCESS;
}

int CGI_Output_Html_End(char *start, char *filename, char *token, int (*replace1)(FILE *fptr, char *oldstr, void *arg1), void *arg1)
{
	FILE	*fptr;
	char    line[CGI_LINE];
	int	ret=CGI_REPLACE_FAIL, find_start = 0;
	char	*ptr, *next, *val;

	if ((fptr=fopen(filename, "r"))==NULL)
		return CGI_HTML_NOT_FOUND;
	while (!feof(fptr))
	{
		if (fgets(line, CGI_LINE, fptr)!=NULL)
		{
			ptr=line;
                        if(strstr(line, start))
                            find_start = 1;

                        if(!find_start)
                            continue;
			while ((val=strstrip(ptr, token, &next))!=NULL)
			{
				fprintf(stdout, "%s", ptr);
				ret=CGI_REPLACE_FAIL;
				if (replace1!=NULL)
					ret=replace1(stdout, val, arg1);
				if (ret==CGI_REPLACE_FAIL)
					ret=global_replace(stdout, val);
				if (ret==CGI_REPLACE_FAIL)
					fprintf(stdout, "%s%s%s", token, val, token);
				ptr=next;
			}
			fprintf(stdout, "%s", ptr);
		}
	}
	fclose(fptr);
	return CGI_SUCCESS;
}

INPUT *CGI_Get_Parameter(INPUT *input, char *name)
{
	INPUT	*arg=NULL, *start=NULL, *ptr;
	int	count=0;

	ptr=input;
	while (ptr!=NULL)
	{
		if (ptr->name && !strcmp(name, ptr->name))
		{
			if (count==0)
			{
				if ((arg=calloc(1, sizeof(INPUT)))==NULL)
					goto get_parameter_error;
				start=arg;
				count++;
			}
			else
			{
				if ((arg->next=calloc(1, sizeof(INPUT)))==NULL)
					goto get_parameter_error;
				arg=(INPUT *)arg->next;
			}
			if ((arg->name=calloc(1, strlen(ptr->name)+1))==NULL)
				goto get_parameter_error;
			if ((arg->val=calloc(1, strlen(ptr->val)+1))==NULL)
				goto get_parameter_error;
			strcpy(arg->name, ptr->name);
			strcpy(arg->val, ptr->val);

		}
		ptr=(INPUT *)ptr->next;
	}
	return start;
get_parameter_error:
	CGI_Free_Input(start);
	return NULL;
}

INPUT *CGI_Find_Parameter(INPUT *input, char *name)
{
	INPUT	*ptr;

	ptr=input;
	while (ptr!=NULL)
	{
		if (ptr->name && !strcmp(name, ptr->name))
			return ptr;
		ptr=(INPUT *)ptr->next;
	}
	return NULL;
}

void CGI_Get_Http_Info(HTTP_INFO *http_info)
{
	char	*ptr;
	long long content_len = 0;
	memset(http_info, 0, sizeof(HTTP_INFO));
	ptr=(char*)getenv("REMOTE_USER");
	if (ptr!=NULL)
		strncpy(http_info->remote_user, ptr, sizeof(http_info->remote_user)-1);
	else
		strncpy(http_info->remote_user, "", sizeof(http_info->remote_user)-1);

	ptr=(char*)getenv("HTTP_X_FORWARDED_FOR");
	if ((ptr!=NULL) && strlen(ptr) > 1) {
		char *tmp_chr = NULL;
		strncpy(http_info->remote_addr, ptr, sizeof(http_info->remote_addr)-1);
		tmp_chr = strchr(http_info->remote_addr, ',');
		if (tmp_chr != (char*) 0) {
			*(tmp_chr) = '\0';
		}
	}
	else {
		ptr=(char*)getenv("REMOTE_ADDR");
		if (ptr!=NULL) {
			if (IS_IPv4_Address(ptr) || IS_IPv6_Address(ptr)) {
				strncpy(http_info->remote_addr, ptr, sizeof(http_info->remote_addr)-1);
			} else {
				strncpy(http_info->remote_addr, "", sizeof(http_info->remote_addr)-1);
			}
		}
		else {
			strncpy(http_info->remote_addr, "", sizeof(http_info->remote_addr)-1);
		}
	}
	ptr=(char*)getenv("CONTENT_LENGTH");
	if (ptr!=NULL){
		content_len = atoll(ptr);
		//__dbg("CONTENT_LENGTH=%s, content_len=%lld\n", ptr, content_len);
		if(content_len > 2147483647) //2^31 - 1
			http_info->content_length=atoll(ptr);
		else
			http_info->content_length=atoi(ptr);
	}
	else
		http_info->content_length=0;

	ptr=(char *)getenv("CONTENT_TYPE");
	if (ptr!=NULL)
	{
		if (strstr(ptr, "multipart/form-data"))
			http_info->content_type=CGI_TYPE_UPLOAD;
		else
			http_info->content_type=CGI_TYPE_NORMAL;
	}
	else
		http_info->content_type=CGI_TYPE_NORMAL;

	ptr=(char*)getenv("HTTP_USER_AGENT");
	if (ptr!=NULL)
	{
		/* added by William Kao */
                if (strstr(ptr, "Safari"))
                        http_info->browser=CGI_SAFARI;
		else
		if (strstr(ptr, "MSIE") && strstr(ptr, "Mac"))
			http_info->browser=CGI_MSIE_MAC;
		else
		if (strstr(ptr, "MSIE"))
			http_info->browser=CGI_MSIE;
		else
		if (strstr(ptr, "Mozilla") && strstr(ptr, "Macintosh"))
			http_info->browser=CGI_MOZILLA_MAC;
		else
		if (strstr (ptr, "Mozilla") && strstr (ptr, "Linux"))
			http_info->browser=CGI_MOZILLA_LINUX;
		else
		if (strstr(ptr, "Mozilla"))
			http_info->browser=CGI_MOZILLA;
		else
			http_info->browser=CGI_UNKNOWN;
	}
	else
		http_info->browser=CGI_UNKNOWN;

	ptr=(char*)getenv("HTTP_HOST");
	if (ptr!=NULL)
		strncpy(http_info->host_addr, ptr, sizeof(http_info->host_addr)-1);
	else
		strncpy(http_info->host_addr, "", sizeof(http_info->host_addr)-1);

	ptr=(char*)getenv("REMOTE_PWD");
	if (ptr!=NULL)
		strncpy(http_info->pwd, ptr, sizeof(http_info->pwd)-1);
	else
		strncpy(http_info->pwd, "", sizeof(http_info->pwd)-1);

	return;
}

#define CGI_CATEGORY_NVR_CFG    0
#define CGI_CATEGORY_SYS_CFG    1

#define AUTH_NORMAL_SUCC        0
#define AUTH_NVR_SUCC           1
#define AUTH_SYS_SUCC           2
#define AUTH_ALL_SUCC           3

void CGI_Check_User()
{
	HTTP_INFO	http_info;
	CGI_Get_Http_Info(&http_info);

#ifdef _VIOSTOR_  //for NVR X86 use

	if( !strcmp(http_info.remote_user, "") ){
                CGI_Init();
                CGI_Load_Html("/cgi-bin/logout.cgi?logout=yes");
                exit(0);
        }
        if ( !User_Belongs_To_Group(http_info.remote_user, GROUP_ADMINISTRATORS) &&
             !User_Belongs_To_Group(http_info.remote_user, GROUP_SYSMGRS) )
        {
		//normal user only can config self's passwd
		char cmd[1024];
                CGI_Init();
		sprintf(cmd,"/cgi-bin/edit_user.cgi?OLD_NAME=%s&function=USER&subfun=MAIN&USERNAME=%s",http_info.remote_user, http_info.remote_user);
                CGI_Load_Html(cmd);
                exit(0);
        }
#if 0
	if(  http_info.cgi_category==CGI_CATEGORY_NVR_CFG && (User_Belongs_To_Group(http_info.remote_user, GROUP_ADMINISTRATORS)|| User_Belongs_To_Group(http_info.remote_user, GROUP_SYSMGRS) || User_Belongs_To_Group2(http_info.remote_user, GROUP_ADMINISTRATORS)|| User_Belongs_To_Group2(http_info.remote_user, GROUP_SYSMGRS)))
        {
                return;
        }
        else if(  http_info.cgi_category==CGI_CATEGORY_SYS_CFG && (User_Belongs_To_Group2(http_info.remote_user, GROUP_ADMINISTRATORS)|| User_Belongs_To_Group2(http_info.remote_user, GROUP_SYSMGRS)))
        {
                return;
        }
        else
        {
                CGI_Init();
                CGI_Load_Html("/cgi-bin/logout.cgi?logout=yes");
                exit(0);
        }
#endif
#else
	if (!User_Belongs_To_Group(http_info.remote_user, GROUP_ADMINISTRATORS))
	{
		CGI_Init();
		CGI_Load_Html("/cgi-bin/logout.cgi?logout=yes");
		exit(0);
	}
#endif
}

void CGI_Init()
{
#if _VIOSTOR_
	fprintf(stdout,"Content-type: text/html%c%c\n", 10, 10);
#else
	fprintf(stdout,"Content-type: text/html%c%c%c%c", 13, 10, 13, 10);
#endif
}

void XML_Init()
{
	fprintf(stdout,"Content-type: text/xml%c%c%c%c", 13, 10, 13, 10);
}

static void my_thread_sleep(int ms)
{
	struct timeval tv;
	struct timespec ts;
	int n;

	gettimeofday(&tv, NULL);

	ts.tv_sec = time(NULL) + ms / 1000;
	ts.tv_nsec = tv.tv_usec * 1000 + 1000 * 1000 * (ms % 1000);
	ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000);
	ts.tv_nsec %= (1000 * 1000 * 1000);

	pthread_mutex_lock(&sleep_mutex);
	n = pthread_cond_timedwait(&sleep_cond, &sleep_mutex, &ts);
	pthread_mutex_unlock(&sleep_mutex);
	if(n == ETIMEDOUT){
		//__dbg("timeout\n");
	}
}


int CGI_Get_Text_IP(char *addr, int len, INPUT *input, char *name1, char *name2, char *name3, char *name4)
{
	INPUT	*tmp;
	int	cnt=0;
	char tmp_str[8] = {0};

	len--;
	if ((tmp=CGI_Find_Parameter(input, name1))==NULL)
	{
		cnt+=2;
		if (cnt>len)
			return CGI_FAIL;
		strcpy(addr, "0.");
	}
	else
	{
		if (atoi(tmp->val)==0)
			cnt+=2;
		else
			cnt=cnt+strlen(tmp->val);
		if (cnt>len)
			return CGI_FAIL;
		sprintf(addr, "%d.", atoi(tmp->val));
	}
	if ((tmp=CGI_Find_Parameter(input, name2))==NULL)
	{
		cnt+=2;
		if (cnt>len)
			return CGI_FAIL;
		strcat(addr, "0.");
	}
	else
	{
		if (atoi(tmp->val)==0)
			cnt+=2;
		else
			cnt=cnt+strlen(tmp->val);
		if (cnt>len)
			return CGI_FAIL;
		memset(tmp_str, 0, 8);
		sprintf(tmp_str, "%d.", atoi(tmp->val));
		strcat(addr, tmp_str);
	}
	if ((tmp=CGI_Find_Parameter(input, name3))==NULL)
	{
		cnt+=2;
		if (cnt>len)
			return CGI_FAIL;
		strcat(addr, "0.");
	}
	else
	{
		if (atoi(tmp->val)==0)
			cnt+=2;
		else
			cnt=cnt+strlen(tmp->val);
		if (cnt>len)
			return CGI_FAIL;
		memset(tmp_str, 0, 8);
		sprintf(tmp_str, "%d.", atoi(tmp->val));
		strcat(addr, tmp_str);
	}
	if ((tmp=CGI_Find_Parameter(input, name4))==NULL)
	{
		cnt+=2;
		if (cnt>len)
			return CGI_FAIL;
		strcat(addr, "0");
	}
	else
	{
		if (atoi(tmp->val)==0)
			cnt+=2;
		else
			cnt=cnt+strlen(tmp->val);
		if (cnt>len)
			return CGI_FAIL;
		memset(tmp_str, 0, 8);
		sprintf(tmp_str, "%d", atoi(tmp->val));
		strcat(addr, tmp_str);
	}
	return CGI_SUCCESS;
}

void cgi_check_timeout(void *arg)
{
	UPLOAD	*upload;
	int	uexit=0;
	char	dest[CGI_LINE];

	upload=(UPLOAD *)arg;
	pthread_t main_thread = *(pthread_t *)upload->main_thread;
	while (1)
	{

		if (upload->predata!=upload->nowdata)
		{
			uexit=0;
			upload->predata=upload->nowdata;
		}
		else
		{
			if (uexit==2)
			{
				/* internet error, so kill myself */
				if (upload->type==CGI_USE_DEFINE)
					sprintf(dest, "%s/%s", upload->path, upload->filename);
				else
					sprintf(dest, "%s/%s", upload->path, upload->src);
#ifdef RECYCLE_EX
				qnap_unlink(dest);
#else
				unlink(dest);
#endif

				// for WFM2 upload fail
				if(upload->wfm2Enable == 1 && upload->tmpfile != NULL) {
					upload->wfm2Enable = 0;
#ifdef RECYCLE_EX
					qnap_unlink(upload->tmpfile);
#else
					unlink(upload->tmpfile);
#endif
					free(upload->tmpfile);
				}
				pthread_kill(main_thread, SIGUSR1);
				pthread_exit(NULL);
			}
			uexit++;
		}
		my_thread_sleep(50000);
	}
}

int wfm2_save_file(UPLOAD *upload, char *src)
{
	FILE            *fptr=NULL;
        char            str[CGI_LINE*128], *ptr=NULL, boundary[CGI_BOUNDARY_LENGTH], tmp[2], limited_file[256];
        char            dest[CGI_LINE], tmp_dest[CGI_LINE], tmpfile[255]={".wfm_uploadXXXXXX"};
        long long       len, total_size = 0;
        long long       i, j, total=0, count;
        int             pre_status=0, oneblock=CGI_LINE*128, read_line = 0, get_line = 0, percent = 0, pre_percent = 0, ppre_status=0;
        HTTP_INFO       http_info;
        struct stat 	buf;
        int str_size = sizeof(str)-1;

	//mode for https upload
	if (!strcmp(upload->mode, "standard")) {
		read_line = 4;
		get_line = 1;
	}
	//for http upload
	else {
		read_line = 8;
		get_line = 5;
	}
        CGI_Get_Http_Info(&http_info);

	//if you get less chars than len, the client browser will show error!!!!!
	len=http_info.content_length;	
	//__dbg("wfm2_save_file: len=%lld\n", len);
	//check if the html is the upload format
	//if not, you shouldn't do anything!!!!!
	if (http_info.content_type!=CGI_TYPE_UPLOAD)
        {       //you can put error message here !!!!!
                for (i=0;i<len;i++)
                        fread(tmp, 1, 1, stdin);
                return CGI_FAIL;
        }

	for (i=0;i<read_line;i++)
        {
                ptr=str;
                memset(str, '\0', sizeof(str));
                count=0;
                do
                {
                        if (fread(ptr,1,1,stdin)==0)
                                goto internet_error;
                        if (i==0)       //first line is the boundary, save it!!
                                boundary[count]=ptr[0];
                        count++;
                        if(count >= str_size){
                                goto internet_error;
                        }
                } while (*ptr++!=0x0a);
                total=total+count;
                if (i==0)               //set string end to boundary
                {
                        if (total >= CGI_BOUNDARY_LENGTH) goto internet_error;
                        if (boundary[total-2]==0x0d)
                                boundary[total-2]=0x0;
                        else
                                boundary[total]=0x0;
                }

                if (i==get_line)               //find the upload filename
                {
			ptr = strstr(str, "; filename=\"");
			if(!ptr){
				ptr = strstr(str, "; filename*=");

				if(ptr)
					ptr+=12;

				/* RFC5987 expected is filename*=UTF-8''xxxxxxxx */
				if(ptr && (!strncmp(ptr, "utf-8''", 7) || !strncmp(ptr, "UTF-8''", 7))){
					ptr=ptr+7;
					if(ptr[strlen(ptr)-1] == '\n')
						ptr[strlen(ptr)-1] = '\0';
					if(ptr[strlen(ptr)-1] == 13)
						ptr[strlen(ptr)-1] = '\0';
				}
			}else{
				ptr +=12;
				if(ptr[strlen(ptr)-1] == '\n')
					ptr[strlen(ptr)-1] = '\0';
				if(ptr[strlen(ptr)-1] == 13)
					ptr[strlen(ptr)-1] = '\0';
				if(ptr[strlen(ptr)-1] == '"')
					ptr[strlen(ptr)-1] = '\0';
			}

			if(ptr){
				strcpy(src, ptr);
				trans_http_str(src, src, 1);
			}
		}
        }

        //len is the chars that you remain want to get !!!
        len=len-total;

	//not input filename, so don't save file...just read
	if (!strcmp(src, "") || src[0]==0x0d)
		goto read_remain;

	//strcpy(upload->src, src);
	if (upload->type==CGI_USE_DEFINE) {
		//fixed bug 16368
		snprintf(limited_file, sizeof(limited_file), "%s", upload->filename);
		snprintf(dest, sizeof(dest), "%s/%s\n", upload->path, limited_file);
		if (!strcmp(upload->mode, "standard"))
			Set_Private_Profile_String("", UPLOAD_FILE_NAME, upload->filename, upload->progress);
	}
	else {
		//fixed bug 16368
		snprintf(limited_file, sizeof(limited_file), "%s", src);
		snprintf(dest, sizeof(dest), "%s/%s", upload->path, limited_file);
		if (!strcmp(upload->mode, "standard"))
			Set_Private_Profile_String("", UPLOAD_FILE_NAME, src, upload->progress);
	}

	snprintf(tmp_dest, sizeof(tmp_dest), "%s/%s", upload->path, tmpfile);
	{
	int fd = mkstemp(tmp_dest);
	if (fd == -1)
		goto open_error;
	close(fd);

	// for wfm2 upload
	upload->tmpfile = strdup(tmp_dest);
	upload->wfm2Enable = 1;
	}

	fptr = fopen(tmp_dest, "w");
	if (fptr == NULL)
		goto open_error;

	/* for thread use */
        total_size = upload->nowdata=len;

	//__dbg("wfm2_save_file: total_size=%lld\n", total_size);
	if (!strcmp(upload->mode, "standard")) {
		Set_Private_Profile_String("", UPLOAD_TMP, basename(upload->tmpfile), upload->progress);
		Set_Private_Profile_Longlong("", UPLOAD_FILE_SIZE, total_size, upload->progress);
	}

        while (1)
        {
                total=0;
                if (len<(oneblock*2))
                {
                        for (j=0;j<oneblock;j++)
                        {
                                total++;
                                if (fread(str+j, 1, 1, stdin)==0)
                                        goto internet_error;

                                if (str[j]==0x0a)
				{
					j++;
                                        break;
				}
                        }

			/* Client will append EOF (0x0d 0x0a) before the boundary string. 
			   If the last byte is 0x0d, it may be the first byte of EOF signal */
                        if (str[j-1]==0x0d)
			{
				ppre_status=1;
			}
                }
                else
                {
                        if ((total=fread(str, 1, oneblock, stdin))<oneblock)
                                goto internet_error;
                }
                len=len-total;
                upload->nowdata=len;         /* for thread use */
                if (len<0)
                        goto internet_error;
                if (str[total-1]==0x0a)
                {
                        if (strstr(str, boundary))
                        {
				if (!strcmp(upload->mode, "standard"))
					Set_Private_Profile_Integer("", UPLOAD_PERCENT, 100, upload->progress);
                                break;
                        }


                        if (total>2)
                        {
                                if (pre_status==1)
                                        fwrite(tmp, 1, 1, fptr);
                                if (pre_status==2)
                                        fwrite(tmp, 2, 1, fptr);
                                fwrite(str, total-2, 1, fptr);
                                pre_status=2;
                                tmp[0]=str[total-2];
                                tmp[1]=str[total-1];
                        }
                        else
                        if (total==2)
                        {
                                if (pre_status==1)
                                        fwrite(tmp, 1, 1, fptr);
                                if (pre_status==2)
                                        fwrite(tmp, 2, 1, fptr);
                                pre_status=2;
                                tmp[0]=str[total-2];
                                tmp[1]=str[total-1];
                        }
                        else
                        {
				if (pre_status==2)
				{
                                        fwrite(tmp, 2, 1, fptr);
					pre_status=0;
				}

				/* pre_status = 1 means we have a byte [0x0d] wait to write. 
				   In this case [0x0a] is followed [0x0d], it is match to EOF string. 
				   We change pre_status to 2 and write those two byte in next round. */
                                if (pre_status==1)
				{
					pre_status= 2;
					tmp[1]=str[total-1];
				}
				else
				{
					fwrite(str, total, 1, fptr);
				}
                        }
                }
                else
                {
                        if (pre_status==1)
                                fwrite(tmp, 1, 1, fptr);
                        if (pre_status==2)
				fwrite(tmp, 2, 1, fptr);
			
                        pre_status=0;
                        fwrite(str, total-ppre_status, 1, fptr);
                }

		/* There are a byte [0x0d] wait to write. Due to the round is break by meet [0x0a], 
		   it is impossible that str include [0x0a 0x0d] (if ppre_status = 1 => pre_status always 0) */
		if(ppre_status)
		{
			pre_status=1;
                        tmp[0]=0x0d;
			ppre_status=0;
		}

		if (!strcmp(upload->mode, "standard")) {
			pre_percent = percent;
			percent = (total_size - len)*100/total_size;
			if (percent >= 100 || percent > pre_percent)
				Set_Private_Profile_Integer("", UPLOAD_PERCENT, percent, upload->progress);
		}
        }
        if (fclose(fptr) < 0)
		goto open_error;
	
	//if mode = skip
	if (!upload->overwrite) {
		//file exist.
                if (lstat(dest, &buf) == 0) {
#ifdef RECYCLE_EX
			qnap_unlink(tmp_dest);
#else
			unlink(tmp_dest);
#endif
                }
		//file do not exist
		else {
			rename(tmp_dest, dest);
		}
        }
	else {
		rename(tmp_dest, dest);
	}

	if(upload->wfm2Enable == 1 && upload->tmpfile != NULL) {
		upload->wfm2Enable = 0;
		free(upload->tmpfile);
	}

read_remain:
        for (i=0;i<len;i++)
                fread(tmp, 1, 1, stdin);
        return CGI_SUCCESS;
open_error:
	//unlink(dest);
#ifdef RECYCLE_EX
	qnap_unlink(tmp_dest);
#else
	unlink(tmp_dest);
#endif
	if(upload->wfm2Enable == 1 && upload->tmpfile != NULL) {
		upload->wfm2Enable = 0;
		unlink(upload->tmpfile);
		free(upload->tmpfile);
	}
	return CGI_UPLOAD_QUOTA_ERROR;
internet_error:
	if(fptr) fclose(fptr);
#ifdef RECYCLE_EX
	qnap_unlink(tmp_dest);
#else
	unlink(tmp_dest);
#endif
	if(upload->wfm2Enable == 1 && upload->tmpfile != NULL) {
		upload->wfm2Enable = 0;
#ifdef RECYCLE_EX
		qnap_unlink(upload->tmpfile);
#else
		unlink(upload->tmpfile);
#endif
		free(upload->tmpfile);
	}
        return CGI_FAIL;
#if 0
        for (i=0;i<len;i++)
                fread(tmp, 1, 1, stdin);
        return CGI_UPLOAD_LEN_NOT_ENOUGH;
#endif
}

int wfm2_save_file_for_cb(UPLOAD *upload, char *src, FuncUp_CB func_cb)
{
    FILE        *fptr=NULL;
    char        str[CGI_LINE*128], *ptr=NULL, boundary[CGI_BOUNDARY_LENGTH], tmp[2], limited_file[256];
    char        dest[CGI_LINE], tmp_dest[CGI_LINE], tmpfile[255]={".wfm_uploadXXXXXX"};
    long long       len, total_size = 0;
    long long       i, j, total=0, count;
    int         pre_status=0, oneblock=CGI_LINE*128, read_line = 0, get_line = 0, percent = 0, pre_percent = 0, ppre_status=0;
    HTTP_INFO       http_info;
    struct stat     buf;
    int str_size = sizeof(str)-1;
    size_t	nbytes;

    //mode for https upload
    if (!strcmp(upload->mode, "standard")) {
        read_line = 4;
        get_line = 1;
    }
    //for http upload
    else {
        read_line = 8;
        get_line = 5;
    }
    CGI_Get_Http_Info(&http_info);

    //if you get less chars than len, the client browser will show error!!!!!
    len=http_info.content_length;
    //__dbg("wfm2_save_file: len=%lld\n", len);
    //check if the html is the upload format
    //if not, you shouldn't do anything!!!!!
    if (http_info.content_type!=CGI_TYPE_UPLOAD)
    {   //you can put error message here !!!!!
        for (i=0;i<len;i++) {
            nbytes=fread(tmp, 1, 1, stdin);
            if(nbytes==0) {
                // do nothing !
            }
        }
        return CGI_FAIL;
    }

    for (i=0;i<read_line;i++)
    {
        ptr=str;
        memset(str, '\0', sizeof(str));
        count=0;
        do
        {
            if (fread(ptr,1,1,stdin)==0)
                goto internet_error;
            if (i==0)       //first line is the boundary, save it!!
                boundary[count]=ptr[0];
            count++;
            if(count >= str_size){
                goto internet_error;
            }
        } while (*ptr++!=0x0a);
        total=total+count;
        if (count==2 && read_line == 4)         //this is the last line, so end this loop
            break;
        if (i==0)           //set string end to boundary
        {
            if (total >= CGI_BOUNDARY_LENGTH) goto internet_error;
            if (boundary[total-2]==0x0d)
                boundary[total-2]=0x0;
            else
                boundary[total]=0x0;
        }

        if (i==get_line)           //find the upload filename
        {
	    ptr = strstr(str, "; filename=\"");
	    if(!ptr){
		ptr = strstr(str, "; filename*=");

		if(ptr)
		    ptr+=12;

		/* RFC5987 expected is filename*=UTF-8''xxxxxxxx */
		if(ptr && (!strncmp(ptr, "utf-8''", 7) || !strncmp(ptr, "UTF-8''", 7))){
		    ptr=ptr+7;
		    if(ptr[strlen(ptr)-1] == '\n')
			ptr[strlen(ptr)-1] = '\0';
		    if(ptr[strlen(ptr)-1] == 13)
			ptr[strlen(ptr)-1] = '\0';
		}
	    }else{
		ptr +=12;
		if(ptr[strlen(ptr)-1] == '\n')
		    ptr[strlen(ptr)-1] = '\0';
		if(ptr[strlen(ptr)-1] == 13)
		    ptr[strlen(ptr)-1] = '\0';
		if(ptr[strlen(ptr)-1] == '"')
		    ptr[strlen(ptr)-1] = '\0';
	    }

            if(ptr){
		strcpy(src, ptr);
                trans_http_str(src, src, 1);
            }
        }
    }

    //len is the chars that you remain want to get !!!
    len=len-total;

    //not input filename, so don't save file...just read
    if (!strcmp(src, "") || src[0]==0x0d)
        goto read_remain;

    //strcpy(upload->src, src);
    if (upload->type==CGI_USE_DEFINE) {
        //fixed bug 16368
        strncpy(limited_file, upload->filename, CGI_UPLOAD_FILENAME_LENGTH);
        sprintf(dest, "%s/%s\n", upload->path, limited_file);
        if (!strcmp(upload->mode, "standard"))
            Set_Private_Profile_String("", UPLOAD_FILE_NAME, upload->filename, upload->progress);
    }
    else {
        //fixed bug 16368
        strncpy(limited_file, src, CGI_UPLOAD_FILENAME_LENGTH);
        sprintf(dest, "%s/%s", upload->path, limited_file);
        if (!strcmp(upload->mode, "standard"))
            Set_Private_Profile_String("", UPLOAD_FILE_NAME, src, upload->progress);
    }

    snprintf(tmp_dest, sizeof(tmp_dest), "%s/%s", upload->path, tmpfile);
    {
    int fd = mkstemp(tmp_dest);
    if (fd == -1) {
        goto open_error;
    }
    close(fd);

    // for wfm2 upload
    upload->tmpfile = strdup(tmp_dest);
    upload->wfm2Enable = 1;
    }

    fptr = fopen(tmp_dest, "w");
    if (fptr == NULL) {
        goto open_error;
    }

    /* for thread use */
    total_size = upload->nowdata=len;

    //__dbg("wfm2_save_file: total_size=%lld\n", total_size);
    if (!strcmp(upload->mode, "standard")) {
        Set_Private_Profile_String("", UPLOAD_TMP, basename(upload->tmpfile), upload->progress);
        Set_Private_Profile_Longlong("", UPLOAD_FILE_SIZE, total_size, upload->progress);
    }

    while (1)
    {
        total=0;
        if (len<(oneblock*2))
        {
            for (j=0;j<oneblock;j++)
            {
                total++;
                if (fread(str+j, 1, 1, stdin)==0)
                    goto internet_error;

                if (str[j]==0x0a)
                {
                    j++;
                    break;
                }
            }

            /* Client will append EOF (0x0d 0x0a) before the boundary string.
               If the last byte is 0x0d, it may be the first byte of EOF signal */
            if (str[j-1]==0x0d)
            {
                ppre_status=1;
            }
        }
        else
        {
            if ((total=fread(str, 1, oneblock, stdin))<oneblock)
                goto internet_error;
        }
        len=len-total;
        upload->nowdata=len;     /* for thread use */
        if (len<0)
            goto internet_error;
        if (str[total-1]==0x0a)
        {
            if (strstr(str, boundary))
            {
                if (!strcmp(upload->mode, "standard"))
                    Set_Private_Profile_Integer("", UPLOAD_PERCENT, 100, upload->progress);
                break;
            }


            if (total>2)
            {
                if (pre_status==1)
                {
                    fwrite(tmp, 1, 1, fptr);
                    if(func_cb != NULL && upload->cb == 2) { func_cb(0, tmp, 1); }
                }
                if (pre_status==2)
                {
                    fwrite(tmp, 2, 1, fptr);
                    if(func_cb != NULL && upload->cb == 2) { func_cb(0, tmp, 2); }
                }
                fwrite(str, total-2, 1, fptr);
                if(func_cb != NULL && upload->cb == 2) { func_cb(0, str, (total-2)); }
                pre_status=2;
                tmp[0]=str[total-2];
                tmp[1]=str[total-1];
            }
            else
            if (total==2)
            {
                if (pre_status==1)
                {
                    fwrite(tmp, 1, 1, fptr);
                    if(func_cb != NULL && upload->cb == 2) { func_cb(0, tmp, 1); }
                }
                if (pre_status==2)
                {
                    fwrite(tmp, 2, 1, fptr);
                    if(func_cb != NULL && upload->cb == 2) { func_cb(0, tmp, 2); }
                }
                pre_status=2;
                tmp[0]=str[total-2];
                tmp[1]=str[total-1];
            }
            else
            {
                if (pre_status==2)
                {
                    fwrite(tmp, 2, 1, fptr);
                    if(func_cb != NULL && upload->cb == 2) { func_cb(0, tmp, 2); }
                    pre_status=0;
                }

                /* pre_status = 1 means we have a byte [0x0d] wait to write.
                   In this case [0x0a] is followed [0x0d], it is match to EOF string.
                   We change pre_status to 2 and write those two byte in next round. */
                if (pre_status==1)
                {
                    pre_status= 2;
                    tmp[1]=str[total-1];
                }
                else
                {
                    fwrite(str, total, 1, fptr);
                    if(func_cb != NULL && upload->cb == 2) { func_cb(0, str, total); }
                }
            }
        }
        else
        {
            if (pre_status==1)
            {
                fwrite(tmp, 1, 1, fptr);
                if(func_cb != NULL && upload->cb == 2) { func_cb(0, tmp, 1); }
            }
            if (pre_status==2)
            {
                fwrite(tmp, 2, 1, fptr);
                if(func_cb != NULL && upload->cb == 2) { func_cb(0, tmp, 2); }
            }
            pre_status=0;
            fwrite(str, total-ppre_status, 1, fptr);
            if(func_cb != NULL && upload->cb == 2) { func_cb(0, str, total-ppre_status); }
        }

        /* There are a byte [0x0d] wait to write. Due to the round is break by meet [0x0a],
           it is impossible that str include [0x0a 0x0d] (if ppre_status = 1 => pre_status always 0) */
        if(ppre_status)
        {
            pre_status=1;
            tmp[0]=0x0d;
            ppre_status=0;
        }

        if (!strcmp(upload->mode, "standard")) {
            pre_percent = percent;
            percent = (total_size - len)*100/total_size;
            if (percent >= 100 || percent > pre_percent)
                Set_Private_Profile_Integer("", UPLOAD_PERCENT, percent, upload->progress);
        }
    }
    if (fclose(fptr) < 0) {
        goto open_error;
    }

    //if mode = skip
    if (!upload->overwrite) {
        //file exist.
        if (lstat(dest, &buf) == 0) {
#ifdef RECYCLE_EX
            qnap_unlink(tmp_dest);
#else
            unlink(tmp_dest);
#endif
        }
        //file do not exist
        else {
            if(rename(tmp_dest, dest)) {
                // do nothing !
            }
        }
    }
    else {
        // QTSFLST0-1263
        if(func_cb != NULL && upload->cb == 1) {
            WFM_UPLOAD_FILE_INFO_T *pInfo=NULL;
            pInfo = (WFM_UPLOAD_FILE_INFO_T *)calloc(1, sizeof(WFM_UPLOAD_FILE_INFO_T));
            if(lstat(dest, &buf) == 0) {
                pInfo->is_dest_exist=1;
                snprintf(pInfo->owner, sizeof(pInfo->owner), "%d", buf.st_uid);
                snprintf(pInfo->group, sizeof(pInfo->group), "%d", buf.st_gid);
            }
            else {
                pInfo->is_dest_exist=0;
            }

            func_cb(pInfo);

            if(rename(tmp_dest, dest)) {
                // do nothing !
            }
        }
        else {
            if(rename(tmp_dest, dest)) {
                // do nothing !
            }
        }
    }

    if(upload->wfm2Enable == 1 && upload->tmpfile != NULL) {
        upload->wfm2Enable = 0;
        free(upload->tmpfile);
    }

read_remain:
    for (i=0;i<len;i++) {
        nbytes=fread(tmp, 1, 1, stdin);
        if(nbytes==0) {
            // do nothing !
        }
    }

    if(func_cb != NULL && upload->cb == 2) { func_cb(2, tmp, 0); }
    return CGI_SUCCESS;
open_error:
    //unlink(dest);
#ifdef RECYCLE_EX
    qnap_unlink(tmp_dest);
#else
    unlink(tmp_dest);
#endif
    if(upload->wfm2Enable == 1 && upload->tmpfile != NULL) {
        upload->wfm2Enable = 0;
        unlink(upload->tmpfile);
        free(upload->tmpfile);
    }
    return CGI_UPLOAD_QUOTA_ERROR;
internet_error:
    if(fptr) fclose(fptr);
#ifdef RECYCLE_EX
    qnap_unlink(tmp_dest);
#else
    unlink(tmp_dest);
#endif
    if(upload->wfm2Enable == 1 && upload->tmpfile != NULL) {
        upload->wfm2Enable = 0;
#ifdef RECYCLE_EX
        qnap_unlink(upload->tmpfile);
#else
        unlink(upload->tmpfile);
#endif
        free(upload->tmpfile);
    }
    return CGI_FAIL;
#if 0
    for (i=0;i<len;i++)
        fread(tmp, 1, 1, stdin);
    return CGI_UPLOAD_LEN_NOT_ENOUGH;
#endif
}

int cgi_save_file(UPLOAD *upload, char *src)
{
	return cgi_save_file_ex(upload, src, UPLOAD_MAX_FILENAME);
}

int cgi_save_file_ex(UPLOAD *upload, char *src, int src_size)
{

	FILE		*fptr=NULL;
	char		str[CGI_LINE*4 + 1] = {0}, *ptr, boundary[CGI_BOUNDARY_LENGTH] = {0}, tmp[2] = {0};
	char		dest[CGI_LINE] = {0};
	long long	len, total=0;
	long long	i, j, count;
	int             pre_status=0, oneblock=CGI_LINE*4, ppre_status=0;
	HTTP_INFO	http_info;
	int str_size = sizeof(str)-1;

	CGI_Get_Http_Info(&http_info);

	//if you get less chars than len, the client browser will show error!!!!!
	len=http_info.content_length;

	//check if the html is the upload format
	//if not, you shouldn't do anything!!!!!
	if (http_info.content_type!=CGI_TYPE_UPLOAD)
	{	//you can put error message here !!!!!
		for (i=0;i<len;i++)
			fread(tmp, 1, 1, stdin);
		return CGI_FAIL;
	}

	//get the header message from the standard input
	//there is always 4 lines, I don't find more or less than 4 line
	//also you can use this header to see the "boundary"
	//ex:boundary=-----------------------xxxxxxxxx\n
	//   Content-Disposition: form-data; name="filename"; filename="C:\folder\filename"\n
	//   Content-Type: application/x-gzip-compressed\n
	//   \n
	//Note: the filename is IE and netscape is not the same
	//	in netscape, it just show the "filename="filename"
	for (i=0;i<4;i++)
	{
		ptr=str;
		count=0;
		do
		{
			if (fread(ptr,1,1,stdin)==0)
				goto internet_error;
			if (i==0)	//first line is the boundary, save it!!
				boundary[count]=ptr[0];
			count++;
			if(count >= str_size){
				goto internet_error;
			}
		} while (*ptr++!=0x0a);
		total=total+count;
		if (count==2)		//this is the last line, so end this loop
			break;
		if (i==0)		//set string end to boundary
		{
			if (total >= CGI_BOUNDARY_LENGTH) goto internet_error;
			if (boundary[total-2]==0x0d)
				boundary[total-2]=0x0;
			else
				boundary[total]=0x0;
		}
#ifdef _DEBUG
		printf("str%d=%s<br>", i, str);
#endif
		if (i==1)		//find the upload filename
		{
			ptr=strtok(str, ";");
			ptr=strtok(NULL, ";");
			ptr=strtok(NULL, "\n");
			if(ptr) snprintf(str, sizeof(str), "%s", ptr);

			ptr=strtok(str, "\"");
			ptr=strtok(NULL, "\"");
			if(ptr) strncpy(src, ptr, src_size);

			if (http_info.browser==CGI_MOZILLA_MAC)
			{
				/* in MAC netscape, space char will be "%20" */
				trans_http_str(src, src, 1);
			}
		}
	}

#ifdef _DEBUG
	printf("Original Upload filename=%s<br>", src);
#endif
	//len is the chars that you remain want to get !!!
	len=len-total;
	//__dbg("upload->sizelimit=%lld, len=%lld, http_info.content_length=%lld\n",upload->sizelimit, len, http_info.content_length );
	if((upload->sizelimit > 0) && (len > upload->sizelimit)){
		for (i=0;i<len;i++)
			fread(tmp, 1, 1, stdin);
		return CGI_UPLOAD_OVER_SIZE_LIMIT_ERROR;
	}
	//not input filename, so don't save file...just read
	if (!strcmp(src, "") || src[0]==0x0d)
		goto read_remain;

	strncpy(upload->src, src, sizeof(upload->src) - 1);
	if (upload->type==CGI_USE_DEFINE)
		snprintf(dest, sizeof(dest), "%s/%s", upload->path, upload->filename);
	else
		snprintf(dest, sizeof(dest), "%s/%s", upload->path, upload->src);

	fptr=fopen(dest, "wb");
	if (fptr == NULL) {
		goto internet_error;
	}
	
	/* for thread use */
	upload->nowdata=len;

	while (1)
	{
		total=0;
		if (len<(oneblock*2))
		{
			for (j=0;j<oneblock;j++)
			{
				total++;
				if (fread(str+j, 1, 1, stdin)==0)
					goto internet_error;

				if (str[j]==0x0a)
				{
					j++;
					break;
				}
			}

			/* Client will append EOF (0x0d 0x0a) before the boundary string. 
			   If the last byte is 0x0d, it may be the first byte of EOF signal */
                        if (str[j-1]==0x0d)
			{
				ppre_status=1;
			}
		}
		else
		{
			if ((total=fread(str, 1, oneblock, stdin))<oneblock)
				goto internet_error;
		}
		len=len-total;
		upload->nowdata=len;	     /* for thread use */
		if (len<0)
		{
			goto internet_error;
		}
		if (str[total-1]==0x0a)
		{
			str[total] = 0;
			if (strstr(str, boundary))
			{
#ifdef _DEBUG
				printf("boundary=%s<br> len=%ld, total=%ld<br>\n", boundary, len, total);
#endif
				break;
			}
			if (total>2)
			{
				if (pre_status==1)
					fwrite(tmp, 1, 1, fptr);
				if (pre_status==2)
					fwrite(tmp, 2, 1, fptr);
				fwrite(str, total-2, 1, fptr);
				pre_status=2;
				tmp[0]=str[total-2];
				tmp[1]=str[total-1];
			}
			else
			if (total==2)
			{
				if (pre_status==1)
					fwrite(tmp, 1, 1, fptr);
				if (pre_status==2)
					fwrite(tmp, 2, 1, fptr);
				pre_status=2;
				tmp[0]=str[total-2];
				tmp[1]=str[total-1];
			}
			else
			{
				if (pre_status==2)
				{
					fwrite(tmp, 2, 1, fptr);
					pre_status=0;
				}

				/* pre_status = 1 means we have a byte [0x0d] wait to write. 
				   In this case [0x0a] is followed [0x0d], it is match to EOF string. 
				   We change pre_status to 2 and write those two byte in next round. */
                                if (pre_status==1)
				{
					pre_status=2;
					tmp[1]=str[total-1];
				}
				else
				{
					fwrite(str, total, 1, fptr);
				}
			}
		}
		else
		{
			if (pre_status==1)
				fwrite(tmp, 1, 1, fptr);
			if (pre_status==2)
				fwrite(tmp, 2, 1, fptr);
			pre_status=0;
			fwrite(str, total-ppre_status, 1, fptr);
		}

		/* There are a byte [0x0d] wait to write. Due to the round is break by meet [0x0a], 
		   it is impossible that str include [0x0a 0x0d] (if ppre_status = 1 => pre_status always 0) */
		if(ppre_status)
		{
			pre_status=1;
                        tmp[0]=0x0d;
			ppre_status=0;
		}
	}
	//	close the filename
	if(fptr) fclose(fptr);

	//	read the remain char from the standard input
	//	if you don't read finish, client browser will show error
read_remain:
	for (i=0;i<len;i++)
		fread(tmp, 1, 1, stdin);
	return CGI_SUCCESS;

//write_error:
	if(fptr)
		fclose(fptr);
#ifdef RECYCLE_EX
	qnap_unlink(dest);
#else
	unlink(dest);
#endif
	while (len>0)
	{
		if (len<oneblock)
		{
			if ((total=fread(str, 1, len, stdin))<len)
				goto internet_error;
		}
		else
		{
			if ((total=fread(str, 1, oneblock, stdin))<oneblock)
				goto internet_error;
		}
		len=len-total;
	}
	return CGI_UPLOAD_WRITE_ERROR;
internet_error:
	if(fptr) fclose(fptr);
#ifdef RECYCLE_EX
	qnap_unlink(dest);
#else
	unlink(dest);
#endif
	return CGI_FAIL;
#if 0
//size_not_enough:
	for (i=0;i<len;i++)
		fread(tmp, 1, 1, stdin);
	return CGI_UPLOAD_LEN_NOT_ENOUGH;
#endif
}


static void _timeout_signal_handler(int signal) 
{
#ifdef _DEBUG
	printf("interrupt by timeout\n");
#endif
}

static void _init_signal_handler()
{
	sigset_t set;
	struct sigaction sigact;

	sigprocmask(SIG_SETMASK, NULL, &set); 
	sigdelset(&set, SIGUSR1);          
	sigprocmask(SIG_SETMASK, &set, NULL);

	sigact.sa_handler = _timeout_signal_handler;
	sigact.sa_flags = 0;
	sigemptyset(&sigact.sa_mask);
	sigaction(SIGUSR1, &sigact, NULL); 
}

int CGI_Upload(char *path, char *filename, char *src)
{
	pthread_t	thread, main_thread;
	UPLOAD		upload;
	int		ret, type, join = 1;

	strncpy(upload.path, path, sizeof(upload.path) - 1);
	if (filename==NULL)
		type=CGI_USE_ORIGINAL;
	else
	{
		type=CGI_USE_DEFINE;
		strncpy(upload.filename, filename, sizeof(upload.filename) - 1);
	}
	upload.predata=upload.nowdata=0;
	upload.type=type;
	upload.sizelimit=0;
	main_thread = pthread_self();
	upload.main_thread = (void *) &main_thread;
	pthread_mutex_init(&sleep_mutex, NULL);
	pthread_cond_init(&sleep_cond, NULL);

	/* init thread */
	pthread_create(&thread, NULL, (void *)&cgi_check_timeout, &upload);

	/* set timeout handler */
	_init_signal_handler();

	/* save file in dest */
	ret=cgi_save_file(&upload, src);

	if(pthread_cancel(thread) != 0){
		join = 0;
	}

	if(join){
		int uu __attribute__((unused));
		void *vret;
		uu = pthread_join(thread, &vret);
	}
	return ret;
}

int CGI_Upload_Size_Limit(char *path, char *filename, char *src, long long sizelimit) //specify upload file size limit in byte
{
	pthread_t	thread, main_thread;
	UPLOAD		upload;
	int		ret, type, join = 1;

	strncpy(upload.path, path, sizeof(upload.path) - 1);
	if (filename==NULL)
		type=CGI_USE_ORIGINAL;
	else
	{
		type=CGI_USE_DEFINE;
		strncpy(upload.filename, filename, sizeof(upload.filename) - 1);
	}
	upload.predata=upload.nowdata=0;
	upload.type=type;
	upload.sizelimit=sizelimit;
	main_thread = pthread_self();
	upload.main_thread = (void *) &main_thread;
	pthread_mutex_init(&sleep_mutex, NULL);
	pthread_cond_init(&sleep_cond, NULL);
	//__dbg("upload.sizelimit=%lld\n", upload.sizelimit);
	/* init thread */
	pthread_create(&thread, NULL, (void *)&cgi_check_timeout, &upload);

	/* set timeout handler */
	_init_signal_handler();

	/* save file in dest */
	ret=cgi_save_file(&upload, src);

	if(pthread_cancel(thread) != 0){
		join = 0;
	}

	if(join){
		int uu __attribute__((unused));
		void *vret;
		uu = pthread_join(thread, &vret);
	}
	return ret;
}

int WFM2_CGI_Upload(char *path, char *filename, char *src, int overwrite, char *mode, char *progress)
{
	pthread_t       thread, main_thread;
	UPLOAD          upload = {};
	int             ret, type, join = 1;

	snprintf(upload.path, sizeof(upload.path), "%s", path);
	if (filename==NULL)
		type=CGI_USE_ORIGINAL;
	else
	{
		type=CGI_USE_DEFINE;
		snprintf(upload.filename, sizeof(upload.filename), "%s", filename);
	}
	upload.predata=upload.nowdata=0;
	upload.type=type;
	upload.sizelimit=0;
	upload.overwrite = overwrite;
	main_thread = pthread_self();
	upload.main_thread = (void *) &main_thread;
	pthread_mutex_init(&sleep_mutex, NULL);
	pthread_cond_init(&sleep_cond, NULL);
	//mode is for https upload
	if (mode) {
		snprintf(upload.mode, sizeof(upload.mode), "%s", mode);
		if ( progress && strlen(progress) ) {
			upload.progress = strdup(progress);
		}
	}

	/* init thread */
	pthread_create(&thread, NULL, (void *)&cgi_check_timeout, &upload);

	/* set timeout handler */
	_init_signal_handler();

	/* save file in dest */
	ret=wfm2_save_file(&upload, src);

	if (mode) {
		if (upload.progress) free(upload.progress);
	}

	if(pthread_cancel(thread) != 0){
		join = 0;
	}

	if(join){
		int uu __attribute__((unused));
		void *vret;
		uu = pthread_join(thread, &vret);
	}
	return ret;
}

int WFM2_CGI_Upload_for_Cb(char *path, char *filename, char *src, int overwrite, char *mode, char *progress, int cb, FuncUp_CB func_cb)
{
    pthread_t       thread, main_thread;
    UPLOAD      upload;
    int         ret, type, join = 1;

    snprintf(upload.path, sizeof(upload.path), "%s", path);
    if (filename==NULL)
        type=CGI_USE_ORIGINAL;
    else
    {
        type=CGI_USE_DEFINE;
        snprintf(upload.filename, sizeof(upload.filename), "%s", filename);
    }
    upload.predata=upload.nowdata=0;
    upload.type=type;
        upload.sizelimit=0;
    upload.overwrite = overwrite;
    main_thread = pthread_self();
    upload.main_thread = (void *) &main_thread;
    pthread_mutex_init(&sleep_mutex, NULL);
    pthread_cond_init(&sleep_cond, NULL);
    //mode is for https upload
    if (mode) {
        upload.progress = (char *)calloc(1, strlen(progress) + 1);
        snprintf(upload.mode, sizeof(upload.mode), "%s", mode);
        strcpy(upload.progress, progress);
    }

    upload.cb = cb;

    /* init thread */
    pthread_create(&thread, NULL, (void *)&cgi_check_timeout, &upload);

    /* set timeout handler */
    _init_signal_handler();

    /* save file in dest */
    ret=wfm2_save_file_for_cb(&upload, src, func_cb);

    if (mode) {
        if (upload.progress) free(upload.progress);
    }

    if(pthread_cancel(thread) != 0){
        join = 0;
    }

    if(join){
        int uu __attribute__((unused));
        void *vret;
        uu = pthread_join(thread, &vret);
    }
    return ret;
}

int global_replace1(FILE *fptr, char *name, void *arg)
{
	char	*ptr;

	if (!strcmp(name, "cgi_load_html"))
	{
		ptr=(char *)arg;
		fprintf(fptr, ptr);
	}
	else
		return CGI_REPLACE_FAIL;
	return CGI_REPLACE_OK;
}

int CGI_Load_Html(char *filename)
{
	CGI_Output_Html1(CGI_LOAD_HTML, "$", global_replace1, filename);
	return CGI_SUCCESS;
}

/****************************************************/
/*      function  Trim()                            */
/*  Description : Wipe spaces and change-line       */
/*                characters off from two sides of  */
/*                the input string and save it back */
/*      Created : Shone 2006/01/10                  */
/****************************************************/
void Trim(char str[])
{
	char *p;
	int countup;

	if (strlen(str)>0)
	{
		p=str+strlen(str)-1;
		while (*p==' ' || *p=='\t' || *p==0xa || *p == 0xd)
			p--;
		*(p+1)=0x0;
		countup=0;
		while(str[countup]==' ' || str[countup]=='\t')
			countup++;
		p=str+countup;
		strcpy(str,p);
	}
}

static const unsigned char *b64_tbl = (unsigned char *)"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const unsigned char b64_pad = '=';

/* base64 decode a group of 4 input chars into a group of between 0 and
* 3 output chars
*/
static void decode_group(unsigned char output[], int output_buf_size, const unsigned char input[], int *n)
{
	unsigned char *t1,*t2;
	*n = 0;
	
	if (input[0] == b64_pad)
		return;

	if (input[1] == b64_pad) {
		//Warning: orphaned bits ignored.
		return;
	}

	t1 = (unsigned char*)strchr((const char *)b64_tbl, input[0]);
	t2 = (unsigned char*)strchr((const char *)b64_tbl, input[1]);

	if ((t1 == NULL) || (t2 == NULL)) {
		//Warning: garbage found, giving up.
		return;
	}

	output[(*n)++] = ((t1 - b64_tbl) << 2) | ((t2 - b64_tbl) >> 4);
	if (-- output_buf_size <= 0) return ;

	if (input[2] == b64_pad)
		return;

	t1 = (unsigned char*)strchr((const char *)b64_tbl, input[2]);

	if (t1 == NULL) {
		//Warning: garbage found, giving up.
		return;
	}

	output[(*n)++] = ((t2 - b64_tbl) << 4) | ((t1 - b64_tbl) >> 2);
	if (-- output_buf_size <= 0) return ;

	if (input[3] == b64_pad)
		return;

	t2 = (unsigned char*)strchr((const char *)b64_tbl, input[3]);

	if (t2 == NULL) {
		//Warning: garbage found, giving up.
		return;
	}

	output[(*n)++] = ((t1 - b64_tbl) << 6) | (t2 - b64_tbl);

	return;
}

int b64_Decode_Ex(char *dest, int dest_size, const char *src)
{
	int len;
	int outsz = 0;
	int remain_size = dest_size-1;	//-1 for last '\0'

	while (*src) {
		decode_group((unsigned char*)dest + outsz, remain_size, (const unsigned char*)src, &len);
		src += 4;
		outsz += len;
		remain_size -= len;
		if (remain_size <= 0)
			break;
	}
	if (outsz < dest_size)
		dest[outsz] = 0;
	return outsz;
}

/* base64 encode a group of between 1 and 3 input chars into a group of
 * 4 output chars */
static void encode_group(unsigned char output[], const unsigned char input[], int n)
{
	unsigned char ingrp[3];
	
	ingrp[0] = n > 0 ? input[0] : 0;
	ingrp[1] = n > 1 ? input[1] : 0;
	ingrp[2] = n > 2 ? input[2] : 0;
	
	/* upper 6 bits of ingrp[0] */
	output[0] = n > 0 ? b64_tbl[ingrp[0] >> 2] : b64_pad;
	/* lower 2 bits of ingrp[0] | upper 4 bits of ingrp[1] */
	output[1] = n > 0 ? b64_tbl[((ingrp[0] & 0x3) << 4) | (ingrp[1] >> 4)] : b64_pad;
	/* lower 4 bits of ingrp[1] | upper 2 bits of ingrp[2] */
	output[2] = n > 1 ? b64_tbl[((ingrp[1] & 0xf) << 2) | (ingrp[2] >> 6)] : b64_pad;
	/* lower 6 bits of ingrp[2] */
	output[3] = n > 2 ? b64_tbl[ingrp[2] & 0x3f] : b64_pad;
}

int b64_Encode_Ex(char *dest, int dest_size, void *src, int src_len)
{
	int outsz = 0;
	int remain_size = dest_size-1;	//-1 for last '\0'

	while (src_len > 0) {
		if (remain_size < 4)
			return outsz;
		encode_group((unsigned char*)dest + outsz, (const unsigned char*)src, src_len > 3 ? 3 : src_len);
		src_len -= 3;
		src = (char*)src + 3 * sizeof(char);
		outsz += 4;
		remain_size -= 4;
	}
	if (outsz < dest_size)
		dest[outsz] = 0;

	return outsz;
}

int Is_b64_Encode(char *str)
{
	int		ret=1;

	if(!str){
		ret=0;
		goto myreturn;
	}

	while(*str){
		if( NULL == strchr((const char*)b64_tbl, *str) && *str != b64_pad){
			ret=0;
			goto myreturn;
		}
		str++;
	}

myreturn:
	return ret;
}

int Get_Tag_Value_By_CookieStrings(char *inputStr, char *tag, char *value, int value_len)
{
	char *ptr, *p, *q, tmp[1024];
	
	ptr = strstr(inputStr, tag);
	if(ptr!=NULL){
		bzero (value, value_len);
		p = strchr(ptr, '=');
		if(p!=NULL){
			strncpy(tmp, p+1, 1023);
			q = strchr(tmp, ';');
			if(q!=NULL)	*q = '\0';
			strncpy(value, tmp, value_len);
			return 0;
		}
	}
	return 1;
}

int Get_Cookie_Value_By_Tag(char *tag, char *value, int value_len)
{
	char *cookie = NULL;
	
	cookie = (char*)getenv("HTTP_COOKIE");
	if(cookie != NULL){
		if(Get_Tag_Value_By_CookieStrings(cookie, tag, value, value_len) == 0){
			return 0;
		}
	}
	return 1;
}

static char *modblieUserAgent[] = {
	"Android","iPhone","iPod","iPad","Android.+mobile","avantgo","bada/","blackberry","blazer","compal","elaine",
	"fennec","hiptop","iemobile","iris","kindle","lge ","maemo","meego.+mobile","midp","mmp","netfront","opera mobi",
	"opera mini","palm os","phone","pixi/","pre/","plucker","pocket","psp","series40","series60","symbian","treo",
	"up.browser","up.link","vodafone","wap","windows ce","windows phone","xda","xiino","Nexus",
	NULL
};

int isMobileAgent(void)
{
	char *http_user_agent = NULL, subStr[5];
	char *regex = "/1207|6310|6590|3gso|4thp|50[1-6]i|770s|802s|a wa|abac|ac(er|oo|s-)|ai(ko|rn)|al(av|ca|co)|amoi|an(ex|ny|yw)|aptu|ar(ch|go)|as(te|us)|attw|au(di|-m|r |s )|avan|be(ck|ll|nq)|bi(lb|rd)|bl(ac|az)|br(e|v)w|bumb|bw-(n|u)|c55/|capi|ccwa|cdm-|cell|chtm|cldc|cmd-|co(mp|nd)|craw|da(it|ll|ng)|dbte|dc-s|devi|dica|dmob|do(c|p)o|ds(12|-d)|el(49|ai)|em(l2|ul)|er(ic|k0)|esl8|ez([4-7]0|os|wa|ze)|fetc|fly(-|_)|g1 u|g560|gene|gf-5|g-mo|go(.w|od)|gr(ad|un)|haie|hcit|hd-(m|p|t)|hei-|hi(pt|ta)|hp( i|ip)|hs-c|ht(c(-| |_|a|g|p|s|t)|tp)|hu(aw|tc)|i-(20|go|ma)|i230|iac( |-|/)|ibro|idea|ig01|ikom|im1k|inno|ipaq|iris|ja(t|v)a|jbro|jemu|jigs|kddi|keji|kgt( |/)|klon|kpt |kwc-|kyo(c|k)|le(no|xi)|lg( g|/(k|l|u)|50|54|-[a-w])|libw|lynx|m1-w|m3ga|m50/|ma(te|ui|xo)|mc(01|21|ca)|m-cr|me(di|rc|ri)|mi(o8|oa|ts)|mmef|mo(01|02|bi|de|do|t(-| |o|v)|zz)|mt(50|p1|v )|mwbp|mywa|n10[0-2]|n20[2-3]|n30(0|2)|n50(0|2|5)|n7(0(0|1)|10)|ne((c|m)-|on|tf|wf|wg|wt)|nok(6|i)|nzph|o2im|op(ti|wv)|oran|owg1|p800|pan(a|d|t)|pdxg|pg(13|-([1-8]|c))|phil|pire|pl(ay|uc)|pn-2|po(ck|rt|se)|prox|psio|pt-g|qa-a|qc(07|12|21|32|60|-[2-7]|i-)|qtek|r380|r600|raks|rim9|ro(ve|zo)|s55/|sa(ge|ma|mm|ms|ny|va)|sc(01|h-|oo|p-)|sdk/|se(c(-|0|1)|47|mc|nd|ri)|sgh-|shar|sie(-|m)|sk-0|sl(45|id)|sm(al|ar|b3|it|t5)|so(ft|ny)|sp(01|h-|v-|v )|sy(01|mb)|t2(18|50)|t6(00|10|18)|ta(gt|lk)|tcl-|tdg-|tel(i|m)|tim-|t-mo|to(pl|sh)|ts(70|m-|m3|m5)|tx-9|up(.b|g1|si)|utst|v400|v750|veri|vi(rg|te)|vk(40|5[0-3]|-v)|vm40|voda|vulc|vx(52|53|60|61|70|80|81|83|85|98)|w3c(-| )|webc|whit|wi(g |nc|nw)|wmlb|wonu|x700|yas-|your|zeto|zte-/i";
	regex_t preg;
	regmatch_t pmatch[10];
	size_t nmatch = 10;
	int i = 0, rc, cflags = REG_EXTENDED | REG_ICASE;
       
	http_user_agent = (char*)getenv("HTTP_USER_AGENT");
	if(http_user_agent){
		strncpy(subStr, http_user_agent, 4);
		subStr[4] = '\0';
		while(modblieUserAgent[i] != NULL){
			if(strcasestr(http_user_agent, modblieUserAgent[i]) != 0){
				return 1;
			}
			i++;
		}
	
		if( regcomp(&preg, regex, cflags) != 0 ) {
			return 0;
		}
		rc = regexec(&preg, subStr, nmatch, pmatch, 0);
		regfree(&preg);
		if (rc == 0) {
			return 1;	//match
		}
	}
	return 0;
}

static char *tabletUserAgent[] = {
	"iPad","Nexus 7","Nexus 10",
	NULL
};

int isTabletAgent(void)
{
	char *http_user_agent = NULL;
	int i = 0;
       
	http_user_agent = (char*)getenv("HTTP_USER_AGENT");
	if(http_user_agent){
		while(tabletUserAgent[i] != NULL){
			if(strcasestr(http_user_agent, tabletUserAgent[i]) != 0){
				return 1;
			}
			i++;
		}
	}
	return 0;
}

int CGI_Get_Accept_Language(char *retStr, int retlen)
{
	char *accept_language = (char*)getenv("ACCEPT_LANGUAGE");
	char tmpStr[8] = "eng";
	int retLangIdx = Q_ENG;
	if(retlen<=4){
		return -1;
	}
	
	if(accept_language){
		if(strncasecmp(accept_language, "en", 2) == 0)
			strcpy(tmpStr, "eng");
		else if(strncasecmp(accept_language, "zh", 2) == 0){
			if(strncasecmp(accept_language+3, "tw", 2) == 0)
			{strcpy(tmpStr, "cht"); retLangIdx=Q_TCH;}
			else if(strncasecmp(accept_language+3, "cn", 2) == 0)
			{strcpy(tmpStr, "chs"); retLangIdx=Q_SCH;}
		}
		else if(strncasecmp(accept_language, "cs", 2) == 0)
			{strcpy(tmpStr, "cze"); retLangIdx=Q_CZE;}
		else if(strncasecmp(accept_language, "da", 2) == 0)
			{strcpy(tmpStr, "dan"); retLangIdx=Q_DAN;}
		else if(strncasecmp(accept_language, "de", 2) == 0)
			{strcpy(tmpStr, "ger"); retLangIdx=Q_GER;}
		else if(strncasecmp(accept_language, "es", 2) == 0)
			{strcpy(tmpStr, "spa"); retLangIdx=Q_SPA;}
		else if(strncasecmp(accept_language, "fr", 2) == 0)
			{strcpy(tmpStr, "fre"); retLangIdx=Q_FRE;}
		else if(strncasecmp(accept_language, "it", 2) == 0)
			{strcpy(tmpStr, "ita"); retLangIdx=Q_ITA;}
		else if(strncasecmp(accept_language, "ja", 2) == 0)
			{strcpy(tmpStr, "jpn"); retLangIdx=Q_JPN;}
		else if(strncasecmp(accept_language, "ko", 2) == 0)
			{strcpy(tmpStr, "kor"); retLangIdx=Q_KOR;}
		else if(strncasecmp(accept_language, "nb", 2) == 0)
			{strcpy(tmpStr, "nor"); retLangIdx=Q_NOR;}
		else if(strncasecmp(accept_language, "pl", 2) == 0)
			{strcpy(tmpStr, "pol"); retLangIdx=Q_POL;}
		else if(strncasecmp(accept_language, "ru", 2) == 0)
			{strcpy(tmpStr, "rus"); retLangIdx=Q_RUS;}
		else if(strncasecmp(accept_language, "fi", 2) == 0)
			{strcpy(tmpStr, "fin"); retLangIdx=Q_FIN;}
		else if(strncasecmp(accept_language, "sv", 2) == 0)
			{strcpy(tmpStr, "swe"); retLangIdx=Q_SWE;}
		else if(strncasecmp(accept_language, "nl", 2) == 0)
			{strcpy(tmpStr, "dut"); retLangIdx=Q_DUT;}
		else if(strncasecmp(accept_language, "tr", 2) == 0)
			{strcpy(tmpStr, "tur"); retLangIdx=Q_TUR;}
		else if(strncasecmp(accept_language, "th", 2) == 0)
			{strcpy(tmpStr, "tha"); retLangIdx=Q_THA;}
		else if(strncasecmp(accept_language, "hu", 2) == 0)
			{strcpy(tmpStr, "hun"); retLangIdx=Q_HUN;}
		else if(strncasecmp(accept_language, "pt", 2) == 0)
			{strcpy(tmpStr, "por"); retLangIdx=Q_POR;}
		else if(strncasecmp(accept_language, "el", 2) == 0)
			{strcpy(tmpStr, "grk"); retLangIdx=Q_GRK;}
		else if(strncasecmp(accept_language, "ro", 2) == 0)
			{strcpy(tmpStr, "rom"); retLangIdx=Q_ROM;}
	}
	strncpy(retStr, tmpStr, retlen);
	retStr[retlen - 1] = '\0';
	return retLangIdx;
}

int CGI_Get_QLANG_Upper(int idx, char *retStr, int retlen)
{
	if(qLANG_ARRAY[idx].u_lang)
		strncpy(retStr, qLANG_ARRAY[idx].u_lang, retlen);
	else
		strncpy(retStr, qLANG_ARRAY[0].u_lang, retlen);
	return 0;
}

int CGI_Get_QLANG_Lower(int idx, char *retStr, int retlen)
{
	if(qLANG_ARRAY[idx].l_lang)
		strncpy(retStr, qLANG_ARRAY[idx].l_lang, retlen);
	else
		strncpy(retStr, qLANG_ARRAY[0].l_lang, retlen);
	return 0;
}

int CGI_Get_QLANG_Idx(int upper, char *langStr)
{
	int i=0, ret = CGI_FAIL;
	if(!langStr)
		return ret;
	if(upper){
		while(qLANG_ARRAY[i].u_lang){
			if(strcmp(qLANG_ARRAY[i].u_lang,langStr))
				i++;
			else{
				ret = i;
				break;
			}
		}
	}
	else{
		while(qLANG_ARRAY[i].l_lang){
			if(strcmp(qLANG_ARRAY[i].l_lang,langStr))
				i++;
			else{
				ret = i;
				break;
			}
		}
	}
	return ret;
}


char* UTF8_to_URLencode(char* s)
{               char sbuf[1024] = {0};
	int len = strlen(s);
	int num = 0;
	int i;
	unsigned int ch;

	for (i = 0; i < len; i++) {
		ch = s[i];
		ch &= 0xffff;
		if (ch <= 0x007f) {             // other ASCII
			sbuf[num++] = (char)ch;
		}
		else {
			strcat(sbuf, hex[ch & 0xff]);
			num += 3;
		}
	}
	return strdup(sbuf);
}

int CGI_Handle_IE_Filename(char *dest_filename, char *src_filename)
{
	char *src=NULL, *dest=NULL, *tmp_str=NULL;

	src = src_filename;
	dest = dest_filename;
	while (*src != 0x0) {
		if (*src == ';') {
			src++;
			*dest++ = '%';
			*dest++ = '3';
			*dest++ = 'B';
		}
		else if (*src == '#') {
			src++;
			*dest++ = '%';
			*dest++ = '2';
			*dest++ = '3';
		}
		else if (*src == '-') {
			src++;
			*dest++ = '%';
			*dest++ = '2';
			*dest++ = 'D';
		}
		else
			*dest++ = *src++;
	}
	*dest = 0x0;
	tmp_str = UTF8_to_URLencode(dest_filename);
    if(tmp_str) {
        strcpy(dest_filename, tmp_str);
        free(tmp_str);
    }
	return 1;
}


int CGI_Handle_Filename(char *src_filename, char *out_filename, int outlen)
{
	int notIE = 1;
	char filename_tmp[BUF_SIZE] = {0};
	
	if(outlen<=0){
		return -1;
	}
	
	char *ptr = (char*)getenv("HTTP_USER_AGENT");
	if (ptr != NULL) {
		if (strstr(ptr, "MSIE")){
			CGI_Handle_IE_Filename(filename_tmp, src_filename);
			notIE = 0;
		}
	}
	if(notIE){
		snprintf(filename_tmp, sizeof(filename_tmp), "%s", src_filename);
	}
	snprintf(out_filename, outlen, "%s", basename(filename_tmp));
	out_filename[outlen - 1] = '\0';
	return 1;
}

int CGI_Get_Upload_ID(char *path, char *id, int size)
{
    char *filepath = NULL, tmpfile[255]={"tmpXXXXXX"};
    filepath = (char *)malloc(strlen(path) + 32);
    if (filepath == NULL)
        return -1;
    sprintf(filepath, "%s/%s", path, tmpfile);
    {
        int fd = mkstemp(filepath);
        if (fd == -1) {
		free(filepath);
		return -1;
	}
        close(fd);
    }
    snprintf(id, size, "%s", basename(filepath));

    free(filepath);

    return 0;
}

int CGI_Get_Tmp_Upload_ID(char *path, char *id, int size)
{
    char *filepath = NULL, tmpfile[255]={"_tmpXXXXXX"};
    filepath = (char *)malloc(strlen(path) + 32);
    if (filepath == NULL)
        return -1;
    sprintf(filepath, "%s/%s", path, tmpfile);
    {
        int fd = mkstemp(filepath);
        if (fd == -1) {
		free(filepath);
		return -1;
	}
        close(fd);
    }
    snprintf(id, size, "%s", basename(filepath));

    free(filepath);
    return 0;
}

static int wfm2_save_file_ex(UPLOAD *upload, char *src)
{
	FILE            *fptr=NULL;
        char            str[CGI_LINE*128], *ptr=NULL, boundary[CGI_BOUNDARY_LENGTH], tmp[2];
        char            tmp_dest[CGI_LINE];
        long long       len=0, total_size = 0;
        long long       i, j, total=0, count;
        int             pre_status=0, oneblock=CGI_LINE*128, read_line = 0, get_line = 0, percent = 0, pre_percent = 0, write_fail = 0, ppre_status=0;
        HTTP_INFO       http_info;
        int str_size = sizeof(str)-1;

	//mode for https upload
	if (!strcmp(upload->mode, "standard")) {
		read_line = 4;
		get_line = 1;
	}
	else if (!strcasecmp(upload->mode, "OkHttp")) { // for APP upload
		read_line = 10;
		get_line = 6;
	}
	else { //for http upload
		read_line = 8;
		get_line = 5;
	}
        CGI_Get_Http_Info(&http_info);

	//if you get less chars than len, the client browser will show error!!!!!
	len=http_info.content_length;	
	//__dbg("wfm2_save_file: len=%lld\n", len);
	//check if the html is the upload format
	//if not, you shouldn't do anything!!!!!
	if (http_info.content_type!=CGI_TYPE_UPLOAD)
        {       //you can put error message here !!!!!
                for (i=0;i<len;i++)
                        fread(tmp, 1, 1, stdin);
                return CGI_FAIL;
        }

	for (i=0;i<read_line;i++)
        {
                ptr=str;
                memset(str, '\0', sizeof(str));
                count=0;
                do
                {
                        if (fread(ptr,1,1,stdin)==0)
                                goto internet_error;
                        if (i==0)       //first line is the boundary, save it!!
                                boundary[count]=ptr[0];
                        count++;
                        if(count >= str_size){
                                goto internet_error;
                        }
                } while (*ptr++!=0x0a);
                total=total+count;
		if (count==2 && read_line == 4)		//this is the last line, so end this loop
			break;
                if (i==0)               //set string end to boundary
                {
                        if (total >= CGI_BOUNDARY_LENGTH) goto internet_error;
                        if (boundary[total-2]==0x0d)
                                boundary[total-2]=0x0;
                        else
                                boundary[total]=0x0;
                }
                if (i==get_line)               //find the upload filename
                {
			ptr = strstr(str, "; filename=\"");
			if(!ptr){
				ptr = strstr(str, "; filename*=");

				if(ptr)
					ptr+=12;

				/* RFC5987 expected is filename*=UTF-8''xxxxxxxx */
				if(ptr && (!strncmp(ptr, "utf-8''", 7) || !strncmp(ptr, "UTF-8''", 7))){
					ptr=ptr+7;
					if(ptr[strlen(ptr)-1] == '\n')
						ptr[strlen(ptr)-1] = '\0';
					if(ptr[strlen(ptr)-1] == 13)
						ptr[strlen(ptr)-1] = '\0';
				}
			}else{
				ptr +=12;
				if(ptr[strlen(ptr)-1] == '\n')
					ptr[strlen(ptr)-1] = '\0';
				if(ptr[strlen(ptr)-1] == 13)
					ptr[strlen(ptr)-1] = '\0';
				if(ptr[strlen(ptr)-1] == '"')
					ptr[strlen(ptr)-1] = '\0';
			}

			if(ptr){
				strcpy(src, ptr);
				trans_http_str(src, src, 1);
			}
		}
        }

        //len is the chars that you remain want to get !!!
        len=len-total;

	//not input filename, so don't save file...just read
        if (!strcmp(src, "") || src[0]==0x0d)
		goto read_remain;

	//MAX_FILE_LENGTH = 257
	//tmp_dest = (char *)calloc(1, strlen(upload->path) + 257);
        if (upload->type==CGI_USE_DEFINE) {
		//fixed bug 16368
		if (!strcmp(upload->mode, "standard"))
			Set_Private_Profile_String("", UPLOAD_FILE_NAME, upload->filename, upload->progress);
	}
        else {
		//fixed bug 16368
		if (!strcmp(upload->mode, "standard"))
			Set_Private_Profile_String("", UPLOAD_FILE_NAME, src, upload->progress);
	}

	snprintf(tmp_dest, sizeof(tmp_dest), "%s/%s", upload->path, upload->filename);

	// for wfm2 upload
	upload->tmpfile = strdup(tmp_dest);
	upload->wfm2Enable = 1;

	if(upload->resume == 1)
		fptr = fopen(tmp_dest, "rb+");
	else
		fptr = fopen(tmp_dest, "w");

	if (fptr == NULL)
		goto open_error;

	if((upload->resume == 1) && (fseeko(fptr, upload->offset, SEEK_SET) != 0)){
		goto open_error;
	}

	/* for thread use */
        total_size = upload->nowdata=len;

	//__dbg("wfm2_save_file: total_size=%lld\n", total_size);
	if (!strcmp(upload->mode, "standard")) {
		Set_Private_Profile_String("", UPLOAD_TMP, tmp_dest, upload->progress);
		Set_Private_Profile_Longlong("", UPLOAD_FILE_SIZE, total_size, upload->progress);
	}

        while (1)
        {
                total=0;
                if (len<(oneblock*2))
                {
                        for (j=0;j<oneblock;j++)
                        {
                                total++;
                                if (fread(str+j, 1, 1, stdin)==0)
                                        goto internet_error;
				
                                if (str[j]==0x0a)
				{
					j++;
                                        break;
				}
                        }

			/* Client will append EOF (0x0d 0x0a) before the boundary string. 
			   If the last byte is 0x0d, it may be the first byte of EOF signal */
                        if (str[j-1]==0x0d)
			{
				ppre_status = 1;
			}
                }
                else
                {
                        if ((total=fread(str, 1, oneblock, stdin))<oneblock){
                                goto internet_error;
			}
                }
                len=len-total;

                upload->nowdata=len;         /* for thread use */
                if (len<0){
                        goto internet_error;
		}
                if (str[total-1]==0x0a)
                {
                        if (strstr(str, boundary))
                        {
				if (!strcmp(upload->mode, "standard"))
					Set_Private_Profile_Integer("", UPLOAD_PERCENT, 100, upload->progress);
                                break;
                        }
                        if (total>2)
                        {
                                if (pre_status==1)
				{
                                        if(fwrite(tmp, 1, 1, fptr) != 1) 
						write_fail = 1;
				}
                                if (pre_status==2)
				{
                                        if(fwrite(tmp, 2, 1, fptr) != 1) 
						write_fail = 1;
				}
                                if(fwrite(str, total-2, 1, fptr) != 1)
					write_fail = 1;
                                pre_status=2;
                                tmp[0]=str[total-2];
                                tmp[1]=str[total-1];
                        }
                        else
                        if (total==2)
                        {
                                if (pre_status==1)
				{
                                        if(fwrite(tmp, 1, 1, fptr) != 1) 
						write_fail = 1;
				}
                                if (pre_status==2)
				{
                                        if(fwrite(tmp, 2, 1, fptr) != 1) 
						write_fail = 1;
				}
                                pre_status=2;
                                tmp[0]=str[total-2];
                                tmp[1]=str[total-1];
                        }
                        else
                        {
                                if (pre_status==2)
                                {
                                        if(fwrite(tmp, 2, 1, fptr) != 1) 
						write_fail = 1;
                                        pre_status=0;
                                }

				/* pre_status = 1 means we have a byte [0x0d] wait to write. 
				   In this case [0x0a] is followed [0x0d], it is match to EOF string. 
				   We change pre_status to 2 and write those two byte in next round. */
                                if (pre_status==1)
				{
					pre_status=2;
					tmp[1]=str[total-1];
				}
				else
				{
					if(fwrite(str, total, 1, fptr) != 1)
						write_fail = 1;
				}

                        }
                }
                else
                {
                        if (pre_status==1)
                        {
                                if(fwrite(tmp, 1, 1, fptr) != 1)
					write_fail = 1;
                        }
                        if (pre_status==2)
                        {
                                if(fwrite(tmp, 2, 1, fptr) != 1)
					write_fail = 1;
                        }
                        pre_status=0;
                        if(fwrite(str, total-ppre_status, 1, fptr) != 1)
				write_fail = 1;
                }

		/* There are a byte [0x0d] wait to write. Due to the round is break by meet [0x0a], 
		   it is impossible that str include [0x0a 0x0d] (if ppre_status = 1 => pre_status always 0) */
		if(ppre_status)
		{
			pre_status=1;
                        tmp[0]=0x0d;
			ppre_status=0;
		}

		if (!strcmp(upload->mode, "standard")) {
			pre_percent = percent;
			percent = (total_size - len)*100/total_size;
			if (percent >= 100 || percent > pre_percent)
				Set_Private_Profile_Integer("", UPLOAD_PERCENT, percent, upload->progress);
		}
        }
        if (write_fail){
			goto open_error;
		}

	if(upload->wfm2Enable == 1 && upload->tmpfile != NULL) {
		upload->wfm2Enable = 0;
		free(upload->tmpfile);
	}

read_remain:
		if(fptr){
			if(fclose(fptr) != 0){
				fptr=NULL;
#ifdef RECYCLE_EX
				qnap_unlink(tmp_dest);
#else
				unlink(tmp_dest);
#endif
				for (i=0;i<len;i++)
					fread(tmp, 1, 1, stdin);
				return CGI_UPLOAD_QUOTA_ERROR;
			}
			fptr=NULL;
		}
        for (i=0;i<len;i++)
                fread(tmp, 1, 1, stdin);
        return CGI_SUCCESS;
open_error:
	if(fptr){
		fclose(fptr);
		fptr=NULL;
	}
#ifdef RECYCLE_EX
	qnap_unlink(tmp_dest);
#else
	unlink(tmp_dest);
#endif
	if(upload->wfm2Enable == 1 && upload->tmpfile != NULL) {
		upload->wfm2Enable = 0;
		unlink(upload->tmpfile);
		free(upload->tmpfile);
	}
	return CGI_UPLOAD_QUOTA_ERROR;

internet_error:
	if(fptr){
		fclose(fptr);
		fptr=NULL;
	}
	if(upload->resume == 0)
#ifdef RECYCLE_EX
		qnap_unlink(tmp_dest);
#else
		unlink(tmp_dest);
#endif
	if(upload->wfm2Enable == 1 && upload->tmpfile != NULL) {
		upload->wfm2Enable = 0;
		if(upload->resume == 0)
#ifdef RECYCLE_EX
			qnap_unlink(upload->tmpfile);
#else
			unlink(upload->tmpfile);
#endif
		free(upload->tmpfile);
	}
        return CGI_UPLOAD_NO_SUCH_FILE;
}

static int wfm2_save_file_cb(UPLOAD *upload, char *src, FuncUp_CB func_cb)
{
	FILE	    *fptr=NULL;
	char	    str[CGI_LINE*128], *ptr=NULL, boundary[CGI_BOUNDARY_LENGTH], tmp[2];
	char	    tmp_dest[CGI_LINE];
	long long       len=0, total_size = 0;
	long long       i, j, total=0, count;
	int	     pre_status=0, oneblock=CGI_LINE*128, read_line = 0, get_line = 0, percent = 0, pre_percent = 0, write_fail = 0, ppre_status=0;
	HTTP_INFO       http_info;
	int str_size = sizeof(str)-1;
	size_t	nbytes;
	int cnt_per_100M=0; // 100 * 1204 / 256 = 400


	//mode for https upload
	if (!strcmp(upload->mode, "standard")) {
		read_line = 4;
		get_line = 1;
	}
	else if (!strcasecmp(upload->mode, "OkHttp")) { // for APP upload
		read_line = 10;
		get_line = 6;
	}
	else { //for http upload
		read_line = 8;
		get_line = 5;
	}
	CGI_Get_Http_Info(&http_info);

	//if you get less chars than len, the client browser will show error!!!!!
	len=http_info.content_length;	
	//__dbg("wfm2_save_file: len=%lld\n", len);
	//check if the html is the upload format
	//if not, you shouldn't do anything!!!!!
	if (http_info.content_type!=CGI_TYPE_UPLOAD)
	{       //you can put error message here !!!!!
		for (i=0;i<len;i++) {
			nbytes=fread(tmp, 1, 1, stdin);
			if(nbytes==0) { return CGI_FAIL; }
		}
		return CGI_FAIL;
	}

	for (i=0;i<read_line;i++)
	{
		ptr=str;
		memset(str, '\0', sizeof(str));
		count=0;
		do
		{
			if (fread(ptr,1,1,stdin)==0)
				goto internet_error;
			if (i==0)       //first line is the boundary, save it!!
				boundary[count]=ptr[0];
			count++;
			if(count >= str_size){
				goto internet_error;
			}
		} while (*ptr++!=0x0a);
		total=total+count;
		if (count==2 && read_line == 4)		//this is the last line, so end this loop
			break;
		if (i==0)	       //set string end to boundary
		{
			if (total >= CGI_BOUNDARY_LENGTH) goto internet_error;
			if (boundary[total-2]==0x0d)
				boundary[total-2]=0x0;
			else
				boundary[total]=0x0;
		}
		if (i==get_line)	       //find the upload filename
		{
			ptr = strstr(str, "; filename=\"");
			if(!ptr){
				ptr = strstr(str, "; filename*=");

				if(ptr)
					ptr+=12;

				/* RFC5987 expected is filename*=UTF-8''xxxxxxxx */
				if(ptr && (!strncmp(ptr, "utf-8''", 7) || !strncmp(ptr, "UTF-8''", 7))){
					ptr=ptr+7;
					if(ptr[strlen(ptr)-1] == '\n')
						ptr[strlen(ptr)-1] = '\0';
					if(ptr[strlen(ptr)-1] == 13)
						ptr[strlen(ptr)-1] = '\0';
				}
			}else{
				ptr +=12;
				if(ptr[strlen(ptr)-1] == '\n')
					ptr[strlen(ptr)-1] = '\0';
				if(ptr[strlen(ptr)-1] == 13)
					ptr[strlen(ptr)-1] = '\0';
				if(ptr[strlen(ptr)-1] == '"')
					ptr[strlen(ptr)-1] = '\0';
			}

			if(ptr){
				strcpy(src, ptr);
				trans_http_str(src, src, 1);
			}
		}
	}

	//len is the chars that you remain want to get !!!
	len=len-total;

	//not input filename, so don't save file...just read
	if (!strcmp(src, "") || src[0]==0x0d)
		goto read_remain;

	//MAX_FILE_LENGTH = 257
	//tmp_dest = (char *)calloc(1, strlen(upload->path) + 257);
	if (upload->type==CGI_USE_DEFINE) {
		//fixed bug 16368
		if (!strcmp(upload->mode, "standard"))
			Set_Private_Profile_String("", UPLOAD_FILE_NAME, upload->filename, upload->progress);
	}
	else {
		//fixed bug 16368
		if (!strcmp(upload->mode, "standard"))
			Set_Private_Profile_String("", UPLOAD_FILE_NAME, src, upload->progress);
	}

	snprintf(tmp_dest, sizeof(tmp_dest), "%s/%s", upload->path, upload->filename);

	// for wfm2 upload
	upload->tmpfile = strdup(tmp_dest);
	upload->wfm2Enable = 1;

	if(upload->resume == 1)
		fptr = fopen(tmp_dest, "rb+");
	else
		fptr = fopen(tmp_dest, "w");

	if (fptr == NULL)
		goto open_error;

	if((upload->resume == 1) && (fseeko(fptr, upload->offset, SEEK_SET) != 0)){
		goto open_error;
	}

	/* for thread use */
	total_size = upload->nowdata=len;

	//__dbg("wfm2_save_file: total_size=%lld\n", total_size);
	if (!strcmp(upload->mode, "standard")) {
		Set_Private_Profile_String("", UPLOAD_TMP, tmp_dest, upload->progress);
		Set_Private_Profile_Longlong("", UPLOAD_FILE_SIZE, total_size, upload->progress);
	}

	while (1)
	{
		total=0;
		if (len<(oneblock*2))
		{
			for (j=0;j<oneblock;j++)
			{
				total++;
				if (fread(str+j, 1, 1, stdin)==0)
					goto internet_error;
				
				if (str[j]==0x0a)
				{
					j++;
					break;
				}
			}

			/* Client will append EOF (0x0d 0x0a) before the boundary string. 
			   If the last byte is 0x0d, it may be the first byte of EOF signal */
			if (str[j-1]==0x0d)
			{
				ppre_status = 1;
			}
		}
		else
		{
			if ((total=fread(str, 1, oneblock, stdin))<oneblock){
				goto internet_error;
			}
		}
		len=len-total;

		upload->nowdata=len;	 /* for thread use */
		if (len<0){
			goto internet_error;
		}
		if (str[total-1]==0x0a)
		{
			if (strstr(str, boundary))
			{
				if (!strcmp(upload->mode, "standard"))
					Set_Private_Profile_Integer("", UPLOAD_PERCENT, 100, upload->progress);
				break;
			}
			if (total>2)
			{
				if (pre_status==1)
				{
					if(fwrite(tmp, 1, 1, fptr) != 1)
						write_fail = 1;
					else { if(func_cb != NULL) { func_cb(0, tmp, 1); } }
				}
				if (pre_status==2)
				{
					if(fwrite(tmp, 2, 1, fptr) != 1)
						write_fail = 1;
					else { if(func_cb != NULL) { func_cb(0, tmp, 2); } }
				}
				if(fwrite(str, total-2, 1, fptr) != 1)
					write_fail = 1;
				else { if(func_cb != NULL) { func_cb(0, str, (total-2)); } }
				pre_status=2;
				tmp[0]=str[total-2];
				tmp[1]=str[total-1];
			}
			else
			if (total==2)
			{
				if (pre_status==1)
				{
					if(fwrite(tmp, 1, 1, fptr) != 1)
						write_fail = 1;
					else { if(func_cb != NULL) { func_cb(0, tmp, 1); } }
				}
				if (pre_status==2)
				{
					if(fwrite(tmp, 2, 1, fptr) != 1)
						write_fail = 1;
					else { if(func_cb != NULL) { func_cb(0, tmp, 2); } }
				}
				pre_status=2;
				tmp[0]=str[total-2];
				tmp[1]=str[total-1];
			}
			else
			{
				if (pre_status==2)
				{
					if(fwrite(tmp, 2, 1, fptr) != 1)
						write_fail = 1;
					else { if(func_cb != NULL) { func_cb(0, tmp, 2); } }
					pre_status=0;
				}

				/* pre_status = 1 means we have a byte [0x0d] wait to write. 
				   In this case [0x0a] is followed [0x0d], it is match to EOF string. 
				   We change pre_status to 2 and write those two byte in next round. */
				if (pre_status==1)
				{
					pre_status=2;
					tmp[1]=str[total-1];
				}
				else
				{
					if(fwrite(str, total, 1, fptr) != 1)
						write_fail = 1;
					else { if(func_cb != NULL) { func_cb(0, str, total); } }
				}

			}
		}
		else
		{
			if (pre_status==1)
			{
				if(fwrite(tmp, 1, 1, fptr) != 1)
					write_fail = 1;
				else { if(func_cb != NULL) { func_cb(0, tmp, 1); } }
			}
			if (pre_status==2)
			{
				if(fwrite(tmp, 2, 1, fptr) != 1)
					write_fail = 1;
				else { if(func_cb != NULL) { func_cb(0, tmp, 2); } }
			}
			pre_status=0;
			if(fwrite(str, total-ppre_status, 1, fptr) != 1)
				write_fail = 1;
			else { if(func_cb != NULL) { func_cb(0, str, (total-ppre_status)); } }
		}

		/* There are a byte [0x0d] wait to write. Due to the round is break by meet [0x0a], 
		   it is impossible that str include [0x0a 0x0d] (if ppre_status = 1 => pre_status always 0) */
		if(ppre_status)
		{
			pre_status=1;
			tmp[0]=0x0d;
			ppre_status=0;
		}

		if (!strcmp(upload->mode, "standard")) {
			pre_percent = percent;
			percent = (total_size - len)*100/total_size;
			if (percent >= 100 || percent > pre_percent)
				Set_Private_Profile_Integer("", UPLOAD_PERCENT, percent, upload->progress);
		}

		// cnt_per_100M=400; // (100MB * 1024) / 256KB = 400
		if(!(++cnt_per_100M % 400) && upload->progress != NULL) {
			Set_Private_Profile_Integer("", UPLOAD_FILE_SIZE, len, upload->progress);
			pre_percent = percent;
			percent = (total_size - len)*100/total_size;
			if (percent >= 100 || percent > pre_percent)
				Set_Private_Profile_Integer("", UPLOAD_PERCENT, percent, upload->progress);
		}
	}

	if(upload->progress != NULL) {
		Set_Private_Profile_Integer("", UPLOAD_FILE_SIZE, total_size, upload->progress);
		Set_Private_Profile_Integer("", UPLOAD_PERCENT, 100, upload->progress);
	}

	if (write_fail){
			goto open_error;
		}

	if(upload->wfm2Enable == 1 && upload->tmpfile != NULL) {
		upload->wfm2Enable = 0;
		free(upload->tmpfile);
	}

read_remain:
		if(fptr){
			if(fclose(fptr) != 0){
				fptr=NULL;
#ifdef RECYCLE_EX
				qnap_unlink(tmp_dest);
#else
				unlink(tmp_dest);
#endif
				for (i=0;i<len;i++) {
					nbytes=fread(tmp, 1, 1, stdin);
					if(nbytes==0) { return CGI_UPLOAD_QUOTA_ERROR; }
				}
				return CGI_UPLOAD_QUOTA_ERROR;
			}
			fptr=NULL;
		}
	for (i=0;i<len;i++) {
		nbytes=fread(tmp, 1, 1, stdin);
		if(nbytes==0) {
			// do nothing !
		}
	}

	if(func_cb != NULL) { func_cb(2, tmp, 0); }

	return CGI_SUCCESS;
open_error:
	if(fptr){
		fclose(fptr);
		fptr=NULL;
	}
#ifdef RECYCLE_EX
	qnap_unlink(tmp_dest);
#else
	unlink(tmp_dest);
#endif
	if(upload->wfm2Enable == 1 && upload->tmpfile != NULL) {
		upload->wfm2Enable = 0;
		unlink(upload->tmpfile);
		free(upload->tmpfile);
	}
	return CGI_UPLOAD_QUOTA_ERROR;

internet_error:
	if(fptr){
		fclose(fptr);
		fptr=NULL;
	}
	if(upload->resume == 0)
#ifdef RECYCLE_EX
		qnap_unlink(tmp_dest);
#else
		unlink(tmp_dest);
#endif
	if(upload->wfm2Enable == 1 && upload->tmpfile != NULL) {
		upload->wfm2Enable = 0;
		if(upload->resume == 0)
#ifdef RECYCLE_EX
			qnap_unlink(upload->tmpfile);
#else
			unlink(upload->tmpfile);
#endif
		free(upload->tmpfile);
	}
	return CGI_UPLOAD_NO_SUCH_FILE;
}

static void cgi_check_timeout_ex(void *arg)
{
	UPLOAD	*upload;
	int	uexit=0;
	char	dest[CGI_LINE];

	upload=(UPLOAD *)arg;
	pthread_t main_thread = *(pthread_t *)upload->main_thread;

	while (1)
	{

		if (upload->predata!=upload->nowdata)
		{
			uexit=0;
			upload->predata=upload->nowdata;
		}
		else
		{
			if (uexit==2)
			{
				/* internet error, so kill myself */
				if (upload->type==CGI_USE_DEFINE)
					sprintf(dest, "%s/%s", upload->path, upload->filename);
				else
					sprintf(dest, "%s/%s", upload->path, upload->src);

				if(upload->resume == 0)
#ifdef RECYCLE_EX
					qnap_unlink(dest);
#else
					unlink(dest);
#endif

				// for WFM2 upload fail
				if(upload->wfm2Enable == 1 && upload->tmpfile != NULL) {
					upload->wfm2Enable = 0;

					if(upload->resume == 0)
#ifdef RECYCLE_EX
						qnap_unlink(upload->tmpfile);
#else
						unlink(upload->tmpfile);
#endif
					free(upload->tmpfile);
				}
				pthread_kill(main_thread, SIGUSR1);
				pthread_exit(NULL);
			}
			uexit++;
		}
		my_thread_sleep(20000);
	}
}

int WFM2_CGI_Upload_Ex(char *path, char *filename, char *src, int overwrite, 
			    char *mode, char *progress, long long offset, int resume)
{
        pthread_t       thread, main_thread;
        UPLOAD          upload;
        int             ret = CGI_FAIL, type, join = 1;

        snprintf(upload.path, sizeof(upload.path), "%s", path);
        if (filename==NULL)
                type=CGI_USE_ORIGINAL;
        else
        {
                type=CGI_USE_DEFINE;
                snprintf(upload.filename, sizeof(upload.filename), "%s", filename);
        }
        upload.predata=upload.nowdata=0;
        upload.type=type;
	upload.sizelimit=0;
	upload.overwrite = overwrite;
	upload.resume = resume;
        //strcpy(upload.upload_id, src);
	upload.offset=offset;
	main_thread = pthread_self();
	upload.main_thread = (void *) &main_thread;
	pthread_mutex_init(&sleep_mutex, NULL);
	pthread_cond_init(&sleep_cond, NULL);

	//mode is for https upload
	if (mode) {
		if(!strcasecmp(mode, "OkHttp")) {
            snprintf(upload.mode, sizeof(upload.mode), "%s", mode);
			if(progress) {
				upload.progress = (char *)calloc(1, strlen(progress) + 1);
				strcpy(upload.progress, progress);
			}
		}
		else {
			upload.progress = (char *)calloc(1, strlen(progress) + 1);
            snprintf(upload.mode, sizeof(upload.mode), "%s", mode);
			strcpy(upload.progress, progress);
		}
	}

        /* init thread */
        pthread_create(&thread, NULL, (void *)&cgi_check_timeout_ex, &upload);

	/* set timeout handler */
	_init_signal_handler();

        /* save file in dest */
        ret=wfm2_save_file_ex(&upload, src);

	if (mode && strcasecmp(mode, "OkHttp")) {
		if (upload.progress) free(upload.progress);
	}

	if(pthread_cancel(thread) != 0){
		join = 0;
	}
		
	if(join){
		int uu __attribute__((unused));
		void *vret;
		uu = pthread_join(thread, &vret);
	}

        return ret;
}

int WFM2_CGI_Upload_CB(char *path, char *filename, char *src, int overwrite, char *mode, char *progress, long long offset, int resume, FuncUp_CB func_cb)
{
	pthread_t       thread, main_thread;
	UPLOAD	  upload;
	int	     ret = CGI_FAIL, type, join = 1;

    snprintf(upload.path, sizeof(upload.path), "%s", path);
	if (filename==NULL)
		type=CGI_USE_ORIGINAL;
	else
	{
		type=CGI_USE_DEFINE;
        snprintf(upload.filename, sizeof(upload.filename), "%s", filename);
	}
	upload.predata=upload.nowdata=0;
	upload.type=type;
	upload.sizelimit=0;
	upload.overwrite = overwrite;
	upload.resume = resume;
	//strcpy(upload.upload_id, src);
	upload.offset=offset;
	main_thread = pthread_self();
	upload.main_thread = (void *) &main_thread;
	pthread_mutex_init(&sleep_mutex, NULL);
	pthread_cond_init(&sleep_cond, NULL);

	//mode is for https upload
	if (mode) {
		if(!strcasecmp(mode, "OkHttp")) {
            snprintf(upload.mode, sizeof(upload.mode), "%s", mode);
			if(progress) {
				upload.progress = (char *)calloc(1, strlen(progress) + 1);
				strcpy(upload.progress, progress);
			}
		}
		else {
			upload.progress = (char *)calloc(1, strlen(progress) + 1);
            snprintf(upload.mode, sizeof(upload.mode), "%s", mode);
			strcpy(upload.progress, progress);
		}
	}
	else {
		if(progress) upload.progress = strdup(progress);
	}

	/* init thread */
	pthread_create(&thread, NULL, (void *)&cgi_check_timeout_ex, &upload);

	/* set timeout handler */
	_init_signal_handler();

	/* save file in dest */
	ret=wfm2_save_file_cb(&upload, src, func_cb);

	if(upload.progress) QNAP_FREE(upload.progress);

	if(pthread_cancel(thread) != 0){
		join = 0;
	}
		
	if(join){
		int uu __attribute__((unused));
		void *vret;
		uu = pthread_join(thread, &vret);
	}

	return ret;
}

int Is_Valid_Number_Parameter(const char *param)
{
	int len;
	
	if (!param)
		return 0;
	if ((len = strlen(param)) <= 0)
		return 0;
	if (strspn(param, "0123456789") == len)
		return 1;
	return 0;
}

int	Get_Cookie_Ssid(int auth_type, char *ssid, int ssid_len)
{
	int		ret=-1;

	char	buf[BUF_SIZE]={},
			auth_str[16]={};
	int i_is_https = is_https();
			
	if(!ssid){
		goto myreturn;
	}

	// "QTS_SSID=xxx"
	switch(auth_type){
		case	AUTH_TYPE_SYSTEM:
			if (i_is_https)
				strncpy(auth_str, "QTS_SSL_SSID=", sizeof(auth_str)-1);
			else
				strncpy(auth_str, "QTS_SSID=", sizeof(auth_str)-1);
			break;
		case	AUTH_TYPE_FILMANAGER:
			if (i_is_https)
				strncpy(auth_str, "QFS_SSL_SSID=", sizeof(auth_str)-1);
			else
				strncpy(auth_str, "QFS_SSID=", sizeof(auth_str)-1);
			break;
		case	AUTH_TYPE_DOWNLOADSTATION:
			if (i_is_https)
				strncpy(auth_str, "QDS_SSL_SSID=", sizeof(auth_str)-1);
			else
				strncpy(auth_str, "QDS_SSID=", sizeof(auth_str)-1);
			break;
		default:
			goto myreturn;
	}
	if(0 != Get_Cookie_Value_By_Tag(auth_str, buf, sizeof(buf))){
		goto myreturn;
	}

	strncpy(ssid, buf, ssid_len-1);
	ret=0;
myreturn:
	return ret;
}

int Is_CSRF_Vulnerability()
{
	int ret=1;
	char *x_requested_with = NULL;
	
	x_requested_with = (char*)getenv("X_Requested_With");
	if(x_requested_with != NULL && !strcmp(x_requested_with, "XMLHttpRequest")){
		ret=0;
	}

myreturn:
	return ret;
}

static int _is_xml_characters(char c)
{
	// 0x00 - 0x08
	// 0x0b - 0x0c
	// 0x0e - 0x1f
	if (((c >= 0x0)&&(c <= 0x8)) || (c == 0xB) || (c == 0xC) || ((c >= 0xE )&&(c <= 0x1F)))
		return 0;
	else
		return 1;
}

//0 : OK, not 0 : invaleid.
int Invalid_XML_Text(char *text)
{
	int i = 0, text_len = 0;
	if (text == NULL)
		return 2;
	text_len = strlen(text);
	if (text_len < 0)
		return 3;
	for (i = 0; i < text_len; i++) {
		if (_is_xml_characters(text[i]) == 0) {
			return 1;
		}
	}
	return 0;
}
