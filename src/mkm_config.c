#if defined(WIN32)
	#include <windows.h>
	#include <Shlobj.h>
#else
	#include <sys/stat.h>
	#include <errno.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <pwd.h>
#endif

#include <malloc.h>
#include <string.h>

#include "mkm_config.h"
#include "mkm_csv.h"
#include "mkm_error.h"
#include "mkm_input_csv.h"
#include "mkm_input_purchases.h"
#include "mkm_output_text.h"

static mkm_config_column_info g_mkm_config_column_info[] =
{
	{ "cardmarket_id", MKM_CONFIG_COLUMN_TYPE_CSV, 0, MKM_CSV_COLUMN_ID_PRODUCT										},
	{ "price", MKM_CONFIG_COLUMN_TYPE_CSV, 0, MKM_CSV_COLUMN_PRICE													},
	{ "csv_language", MKM_CONFIG_COLUMN_TYPE_CSV, 0, MKM_CSV_COLUMN_ID_LANGUAGE										},
	{ "condition", MKM_CONFIG_COLUMN_TYPE_CSV, 0, MKM_CSV_COLUMN_CONDITION											},
	{ "is_foil", MKM_CONFIG_COLUMN_TYPE_CSV, 0, MKM_CSV_COLUMN_IS_FOIL												},
	{ "is_signed", MKM_CONFIG_COLUMN_TYPE_CSV, 0, MKM_CSV_COLUMN_IS_SIGNED											},
	{ "is_altered", MKM_CONFIG_COLUMN_TYPE_CSV, 0, MKM_CSV_COLUMN_IS_ALTERED										},
	{ "purchase_id", MKM_CONFIG_COLUMN_TYPE_PURCHASE_ID, 0, 0														},
	{ "shipping_cost", MKM_CONFIG_COLUMN_TYPE_PURCHASE_SHIPPING_COST, 0, 0											},
	{ "purchase_date", MKM_CONFIG_COLUMN_TYPE_PURCHASE_DATE, 0, 0													},
	{ "trustee_fee", MKM_CONFIG_COLUMN_TYPE_PURCHASE_TRUSTEE_FEE, 0, 0												},
	{ "tcgplayer_id", MKM_CONFIG_COLUMN_TYPE_SFC_TCGPLAYER_ID, 0, 0													},
	{ "collector_number", MKM_CONFIG_COLUMN_TYPE_SFC_COLLECTOR_NUMBER, 0, 0											},
	{ "color_is_red", MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IS_RED, 0, 0													},
	{ "color_is_blue", MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IS_BLUE, 0, 0												},
	{ "color_is_green", MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IS_GREEN, 0, 0												},
	{ "color_is_black", MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IS_BLACK, 0, 0												},
	{ "color_is_white", MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IS_WHITE, 0, 0												},
	{ "color_identity_is_red", MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IDENTITY_IS_RED, 0, 0								},
	{ "color_identity_is_blue", MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IDENTITY_IS_BLUE, 0, 0								},
	{ "color_identity_is_green", MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IDENTITY_IS_GREEN, 0, 0							},
	{ "color_identity_is_black", MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IDENTITY_IS_BLACK, 0, 0							},
	{ "color_identity_is_white", MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IDENTITY_IS_WHITE, 0, 0							},
	{ "name", MKM_CONFIG_COLUMN_TYPE_SFC_NAME, 0, 0																	},
	{ "set", MKM_CONFIG_COLUMN_TYPE_SFC_SET, 0, 0																	},
	{ "version", MKM_CONFIG_COLUMN_TYPE_SFC_VERSION, 0, 0															},
	{ "released_at", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_RELEASED_AT, 0								},
	{ "rarity", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_RARITY, 0										},
	{ "language", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LANGUAGE, 0									},
	{ "scryfall_id", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_SCRYFALL_ID, 0								},
	{ "layout", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LAYOUT, 0										},
	{ "mana_cost", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_MANA_COST, 0									},
	{ "string_cmc", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_CMC, 0										},
	{ "type_line", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_TYPE_LINE, 0									},
	{ "oracle_text", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_ORACLE_TEXT, 0								},
	{ "reserved", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_RESERVED, 0									},
	{ "foil", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_FOIL, 0											},
	{ "nonfoil", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_NONFOIL, 0										},
	{ "oversized", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_OVERSIZED, 0									},
	{ "promo", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_PROMO, 0											},
	{ "reprint", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_REPRINT, 0										},
	{ "variation", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_VARIATION, 0									},
	{ "set_name", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_SET_NAME, 0									},
	{ "set_type", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_SET_TYPE, 0									},
	{ "digital", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_DIGITAL, 0										},
	{ "flavor_text", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_FLAVOR_TEXT, 0								},
	{ "artist", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_ARTIST, 0										},
	{ "back_id", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_BACK_ID, 0										},
	{ "illustration_id", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_ILLUSTRATION_ID, 0						},
	{ "border_color", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_BORDER_COLOR, 0							},
	{ "frame", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_FRAME, 0											},
	{ "full_art", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_FULL_ART, 0									},
	{ "textless", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_TEXTLESS, 0									},
	{ "booster", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_BOOSTER, 0										},
	{ "power", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_POWER, 0											},
	{ "toughness", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_TOUGHNESS, 0									},
	{ "image_uri_small", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_IMAGE_URI_SMALL, 0						},
	{ "image_uri_normal", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_IMAGE_URI_NORMAL, 0					},
	{ "image_uri_large", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_IMAGE_URI_LARGE, 0						},
	{ "image_uri_png", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_IMAGE_URI_PNG, 0							},
	{ "image_uri_art_crop", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_IMAGE_URI_ART_CROP, 0				},
	{ "image_uri_border_crop", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_IMAGE_URI_BORDER_CROP, 0			},
	{ "recent_price_usd", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_PRICE_USD, 0							},
	{ "recent_price_usd_foil", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_PRICE_USD_FOIL, 0					},
	{ "recent_price_usd_etched", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_PRICE_USD_ETCHED, 0				},
	{ "recent_price_eur", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_PRICE_EUR, 0							},
	{ "recent_price_eur_foil", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_PRICE_EUR_FOIL, 0					},
	{ "recent_price_tix", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_PRICE_TIX, 0							},
	{ "legality_standard", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_STANDARD, 0					},
	{ "legality_future", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_FUTURE, 0						},
	{ "legality_historic", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_HISTORIC, 0					},
	{ "legality_gladiator", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_GLADIATOR, 0				},
	{ "legality_pioneer", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_PIONEER, 0					},
	{ "legality_modern", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_MODERN, 0						},
	{ "legality_legacy", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_LEGACY, 0						},
	{ "legality_pauper", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_PAUPER, 0						},
	{ "legality_vintage", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_VINTAGE, 0					},
	{ "legality_penny", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_PENNY, 0						},
	{ "legality_commander", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_COMMANDER, 0				},
	{ "legality_brawl", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_BRAWL, 0						},
	{ "legality_historicbrawl", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_HISTORICBRAWL, 0		},
	{ "legality_alchemy", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_ALCHEMY, 0					},
	{ "legality_paupercommander", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_PAUPERCOMMANDER, 0	},
	{ "legality_duel", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_DUEL, 0							},
	{ "legality_oldschool", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_OLDSCHOOL, 0				},
	{ "legality_premodern", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_PREMODERN, 0				},

	{ NULL, 0, 0, 0																									}
};

static const mkm_config_column_info*
mkm_config_find_column_info(
	const char*						string)
{
	const mkm_config_column_info* p = g_mkm_config_column_info;
	while(p->name != NULL)
	{
		if(strcmp(p->name, string) == 0)
			return p;

		p++;
	}

	mkm_error("Invalid column: %s", string);
	return NULL;
}

static void
mkm_config_parse_columns(
	mkm_config*						config,
	const char*						string)
{
	char temp[1024];
	mkm_strcpy(temp, string, sizeof(temp));

	char* p = temp;
	mkm_config_column* last_column = NULL;

	for(;;)
	{
		size_t token_length = 0;
		while (p[token_length] != '+' && p[token_length] != '\0')
			token_length++;

		mkm_bool end_of_line = p[token_length] == '\0';

		p[token_length] = '\0';

		if(mkm_config_get_column_index_by_name(config, p) == UINT32_MAX)
		{
			/* Only add column if not already defined */
			mkm_config_column* column = MKM_NEW(mkm_config_column);

			column->info = mkm_config_find_column_info(p);

			if (last_column != NULL)
				last_column->next = column;
			else
				config->columns = column;

			last_column = column;

			config->num_columns++;
		}

		if (end_of_line)
			break;

		p += token_length + 1;
	}
}

void
mkm_config_make_dir(
	const char*			path)
{
	#if defined(WIN32)
		BOOL result = CreateDirectoryA(path, NULL);

		if (result == FALSE && GetLastError() != ERROR_ALREADY_EXISTS)
			mkm_error("Failed to make directory: %s", path);
	#else
		int result = mkdir(path, S_IRWXU);
		if (result != 0 && errno != EEXIST)
			mkm_error("Failed to make directory: %s (%u)", path, errno);
	#endif
}

void
mkm_config_get_default_cache_path(
	char*				buffer,
	size_t				buffer_size)
{
	char home_path[1024];

	#if defined(WIN32)
		{
			PWSTR w_buffer = NULL;
			HRESULT hr = SHGetKnownFolderPath((REFKNOWNFOLDERID)&FOLDERID_RoamingAppData, 0, NULL, &w_buffer);
			MKM_ERROR_CHECK(hr == S_OK, "SHGetKnownFolderPath() failed: %u", hr);

			BOOL used_default_char = FALSE;
			int result = WideCharToMultiByte(CP_ACP, 0, (LPCWCH)w_buffer, -1, (LPSTR)home_path, sizeof(home_path) - 1, NULL, &used_default_char);

			if (w_buffer != NULL)
				CoTaskMemFree(w_buffer);

			MKM_ERROR_CHECK(used_default_char == FALSE && result > 0, "WideCharToMultiByte() failed: %u", GetLastError());

			size_t len = strlen(home_path);
			for (size_t i = 0; i < len; i++)
			{
				if (home_path[i] == '\\')
					home_path[i] = '/';
			}
		}

	#else
		struct passwd* pw = getpwuid(getuid());
		strncpy(home_path, pw->pw_dir, sizeof(home_path));
		/* FIXME: bounds check */
	#endif

	char mkm_csv_dir[1024];
	{
		size_t result = snprintf(mkm_csv_dir, sizeof(mkm_csv_dir), "%s/.mkmcsv", home_path);
		MKM_ERROR_CHECK(result <= sizeof(mkm_csv_dir), "Path too long.");
	}

	mkm_config_make_dir(mkm_csv_dir);

	{
		size_t result = snprintf(buffer, buffer_size, "%s/cache.bin", mkm_csv_dir);
		MKM_ERROR_CHECK(result <= buffer_size, "Path too long.");
	}
}

/*--------------------------------------------------------------------*/

void	
mkm_config_init(
	mkm_config*			config,
	int					argc,
	char**				argv)
{
	memset(config, 0, sizeof(mkm_config));

	config->input_callback = mkm_input_csv;
	config->output_callback = mkm_output_text;

	config->output_stream = stdout;

	mkm_config_input_file* last_input_file = NULL;

	for(int i = 1; i < argc; i++)
	{
		const char* arg = argv[i];

		if(arg[0] == '-')
		{
			if(strcmp(arg, "--columns") == 0)
			{
				i++;
				MKM_ERROR_CHECK(i < argc, "Expected columns after --columns.");
				mkm_config_parse_columns(config, argv[i]);
			}
			else if (strcmp(arg, "--cache") == 0)
			{
				i++;
				MKM_ERROR_CHECK(i < argc, "Expected path after --cache.");
				mkm_strcpy(config->cache_file, argv[i], sizeof(config->cache_file));
			}
			else if(strcmp(arg, "--input") == 0)
			{
				i++;
				MKM_ERROR_CHECK(i < argc, "Expected input type after --input.");
				char* input_type = argv[i];
				if(strcmp(input_type, "csv") == 0)
					config->input_callback = mkm_input_csv;
				else if (strcmp(input_type, "purchases") == 0)
				{
					config->input_callback = mkm_input_purchases;

					/* Add required columns for processing purchases */
					mkm_config_parse_columns(config, "name+version+set+condition+price+shipping_cost+trustee_fee");
				}
				else
					mkm_error("Invalid input type: %s", input_type);
			}
			else if (strcmp(arg, "--output") == 0)
			{
				i++;
				MKM_ERROR_CHECK(i < argc, "Expected output type after --output.");
				char* output_type = argv[i];
				if (strcmp(output_type, "text") == 0)
					config->output_callback = mkm_output_text;
				else
					mkm_error("Invalid output type: %s", output_type);
			}
			else
			{
				mkm_error("Invalid command-line argument: %s", arg);
			}
		}
		else
		{
			/* Add input file */
			mkm_config_input_file* input_file = MKM_NEW(mkm_config_input_file);

			input_file->path = arg;

			if(last_input_file != NULL)
				last_input_file->next = input_file;
			else 
				config->input_files = input_file;

			last_input_file = input_file;
		}
	}

	if(config->columns == NULL)
	{
		/* Add default columns */
		mkm_config_parse_columns(config, "name+version+set+condition+price");
	}

	if(config->cache_file[0] == '\0')
	{
		/* No cache file specified, determine default path */
		mkm_config_get_default_cache_path(config->cache_file, sizeof(config->cache_file));
	}
}

void	
mkm_config_uninit(
	mkm_config*			config)
{
	/* Free input files */
	{
		mkm_config_input_file* input_file = config->input_files;
		while(input_file != NULL)
		{
			mkm_config_input_file* next = input_file->next;
			free(input_file);
			input_file = next;
		}
	}

	/* Free columns */
	{
		mkm_config_column* column = config->columns;
		while (column != NULL)
		{
			mkm_config_column* next = column->next;
			free(column);
			column = next;
		}
	}
}

uint32_t	
mkm_config_get_column_index_by_name(
	const mkm_config*	config,
	const char*			name)
{
	uint32_t index = 0;

	for(const mkm_config_column* column = config->columns; column != NULL; column = column->next)
	{
		if(strcmp(column->info->name, name) == 0)
		{
			assert(index < config->num_columns);
			return index;
		}

		index++;
	}

	return UINT32_MAX;
}
