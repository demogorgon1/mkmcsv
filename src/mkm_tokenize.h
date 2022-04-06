#ifndef __MKM_TOKENIZER_H__
#define __MKM_TOKENIZER_H__

#define MKM_TOKENIZE_MAX	128

typedef struct _mkm_tokenize
{
	size_t		num_tokens;
	char*		tokens[MKM_TOKENIZE_MAX];
} mkm_tokenize;

void	mkm_tokenize_string(
			mkm_tokenize*	tokenize,
			char*			string);

#endif /* __MKM_TOKENIZER_H__ */