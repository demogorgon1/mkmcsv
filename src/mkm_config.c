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
#include <stdlib.h>
#include <string.h>

#include "mkm_config.h"
#include "mkm_csv.h"
#include "mkm_error.h"
#include "mkm_input_csv.h"
#include "mkm_input_shipments.h"
#include "mkm_output_csv.h"
#include "mkm_output_text.h"
#include "mkm_tokenize.h"

static mkm_config_column_info g_mkm_config_column_info[] =
{
	{ "cardmarket_id", MKM_CONFIG_COLUMN_TYPE_CSV, 0, MKM_FALSE, MKM_CSV_COLUMN_ID_PRODUCT									},
	{ "price", MKM_CONFIG_COLUMN_TYPE_CSV, 0, MKM_FALSE, MKM_CSV_COLUMN_PRICE												},
	{ "csv_language", MKM_CONFIG_COLUMN_TYPE_CSV, 0, MKM_FALSE, MKM_CSV_COLUMN_ID_LANGUAGE									},
	{ "condition", MKM_CONFIG_COLUMN_TYPE_CSV, 0, MKM_FALSE, MKM_CSV_COLUMN_CONDITION										},
	{ "is_foil", MKM_CONFIG_COLUMN_TYPE_CSV, 0, MKM_FALSE, MKM_CSV_COLUMN_IS_FOIL											},
	{ "is_signed", MKM_CONFIG_COLUMN_TYPE_CSV, 0, MKM_FALSE, MKM_CSV_COLUMN_IS_SIGNED										},
	{ "is_altered", MKM_CONFIG_COLUMN_TYPE_CSV, 0, MKM_FALSE, MKM_CSV_COLUMN_IS_ALTERED										},
	{ "purchase_id", MKM_CONFIG_COLUMN_TYPE_SHIPMENT_PURCHASE_ID, 0, MKM_FALSE, 0											},
	{ "shipping_cost", MKM_CONFIG_COLUMN_TYPE_SHIPMENT_SHIPPING_COST, 0, MKM_FALSE, 0										},
	{ "purchase_date", MKM_CONFIG_COLUMN_TYPE_SHIPMENT_PURCHASE_DATE, 0, MKM_FALSE, 0										},
	{ "trustee_fee", MKM_CONFIG_COLUMN_TYPE_SHIPMENT_TRUSTEE_FEE, 0, MKM_FALSE, 0											},
	{ "tcgplayer_id", MKM_CONFIG_COLUMN_TYPE_SFC_TCGPLAYER_ID, 0, MKM_FALSE, 0												},
	{ "collector_number", MKM_CONFIG_COLUMN_TYPE_SFC_COLLECTOR_NUMBER, 0, MKM_FALSE, 0										},
	{ "color_is_red", MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IS_RED, 0, MKM_FALSE, 0												},
	{ "color_is_blue", MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IS_BLUE, 0, MKM_FALSE, 0											},
	{ "color_is_green", MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IS_GREEN, 0, MKM_FALSE, 0											},
	{ "color_is_black", MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IS_BLACK, 0, MKM_FALSE, 0											},
	{ "color_is_white", MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IS_WHITE, 0, MKM_FALSE, 0											},
	{ "color_identity_is_red", MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IDENTITY_IS_RED, 0, MKM_FALSE, 0							},
	{ "color_identity_is_blue", MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IDENTITY_IS_BLUE, 0, MKM_FALSE, 0							},
	{ "color_identity_is_green", MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IDENTITY_IS_GREEN, 0, MKM_FALSE, 0						},
	{ "color_identity_is_black", MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IDENTITY_IS_BLACK, 0, MKM_FALSE, 0						},
	{ "color_identity_is_white", MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IDENTITY_IS_WHITE, 0, MKM_FALSE, 0						},
	{ "name", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_NAME, MKM_FALSE, 0											},
	{ "set", MKM_CONFIG_COLUMN_TYPE_SFC_SET, 0, MKM_FALSE, 0																},
	{ "version", MKM_CONFIG_COLUMN_TYPE_SFC_VERSION, 0, MKM_FALSE, 0														},
	{ "released_at", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_RELEASED_AT, MKM_FALSE, 0							},
	{ "rarity", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_RARITY, MKM_FALSE, 0										},
	{ "language", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LANGUAGE, MKM_FALSE, 0									},
	{ "scryfall_id", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_SCRYFALL_ID, MKM_FALSE, 0							},
	{ "layout", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LAYOUT, MKM_FALSE, 0										},
	{ "mana_cost", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_MANA_COST, MKM_FALSE, 0								},
	{ "string_cmc", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_CMC, MKM_FALSE, 0									},
	{ "type_line", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_TYPE_LINE, MKM_FALSE, 0								},
	{ "oracle_text", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_ORACLE_TEXT, MKM_FALSE, 0							},
	{ "reserved", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_RESERVED, MKM_FALSE, 0									},
	{ "foil", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_FOIL, MKM_FALSE, 0											},
	{ "nonfoil", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_NONFOIL, MKM_FALSE, 0									},
	{ "oversized", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_OVERSIZED, MKM_FALSE, 0								},
	{ "promo", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_PROMO, MKM_FALSE, 0										},
	{ "reprint", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_REPRINT, MKM_FALSE, 0									},
	{ "variation", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_VARIATION, MKM_FALSE, 0								},
	{ "set_name", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_SET_NAME, MKM_FALSE, 0									},
	{ "set_type", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_SET_TYPE, MKM_FALSE, 0									},
	{ "digital", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_DIGITAL, MKM_FALSE, 0									},
	{ "flavor_text", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_FLAVOR_TEXT, MKM_FALSE, 0							},
	{ "artist", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_ARTIST, MKM_FALSE, 0										},
	{ "back_id", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_BACK_ID, MKM_FALSE, 0									},
	{ "illustration_id", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_ILLUSTRATION_ID, MKM_FALSE, 0					},
	{ "border_color", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_BORDER_COLOR, MKM_FALSE, 0							},
	{ "frame", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_FRAME, MKM_FALSE, 0										},
	{ "full_art", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_FULL_ART, MKM_FALSE, 0									},
	{ "textless", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_TEXTLESS, MKM_FALSE, 0									},
	{ "booster", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_BOOSTER, MKM_FALSE, 0									},
	{ "power", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_POWER, MKM_FALSE, 0										},
	{ "toughness", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_TOUGHNESS, MKM_FALSE, 0								},
	{ "image_uri_small", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_IMAGE_URI_SMALL, MKM_FALSE, 0					},
	{ "image_uri_normal", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_IMAGE_URI_NORMAL, MKM_FALSE, 0					},
	{ "image_uri_large", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_IMAGE_URI_LARGE, MKM_FALSE, 0					},
	{ "image_uri_png", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_IMAGE_URI_PNG, MKM_FALSE, 0						},
	{ "image_uri_art_crop", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_IMAGE_URI_ART_CROP, MKM_FALSE, 0				},
	{ "image_uri_border_crop", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_IMAGE_URI_BORDER_CROP, MKM_FALSE, 0		},
	{ "recent_price_usd", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_PRICE_USD, MKM_TRUE, 0							},
	{ "recent_price_usd_foil", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_PRICE_USD_FOIL, MKM_TRUE, 0				},
	{ "recent_price_usd_etched", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_PRICE_USD_ETCHED, MKM_TRUE, 0			},
	{ "recent_price_eur", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_PRICE_EUR, MKM_TRUE, 0							},
	{ "recent_price_eur_foil", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_PRICE_EUR_FOIL, MKM_TRUE, 0				},
	{ "recent_price_tix", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_PRICE_TIX, MKM_FALSE, 0						},
	{ "legality_standard", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_STANDARD, MKM_FALSE, 0				},
	{ "legality_future", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_FUTURE, MKM_FALSE, 0					},
	{ "legality_historic", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_HISTORIC, MKM_FALSE, 0				},
	{ "legality_gladiator", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_GLADIATOR, MKM_FALSE, 0				},
	{ "legality_pioneer", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_PIONEER, MKM_FALSE, 0					},
	{ "legality_modern", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_MODERN, MKM_FALSE, 0					},
	{ "legality_legacy", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_LEGACY, MKM_FALSE, 0					},
	{ "legality_pauper", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_PAUPER, MKM_FALSE, 0					},
	{ "legality_vintage", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_VINTAGE, MKM_FALSE, 0					},
	{ "legality_penny", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_PENNY, MKM_FALSE, 0						},
	{ "legality_commander", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_COMMANDER, MKM_FALSE, 0				},
	{ "legality_brawl", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_BRAWL, MKM_FALSE, 0						},
	{ "legality_historicbrawl", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_HISTORICBRAWL, MKM_FALSE, 0		},
	{ "legality_alchemy", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_ALCHEMY, MKM_FALSE, 0					},
	{ "legality_paupercommander", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_PAUPERCOMMANDER, MKM_FALSE, 0	},
	{ "legality_duel", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_DUEL, MKM_FALSE, 0						},
	{ "legality_oldschool", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_OLDSCHOOL, MKM_FALSE, 0				},
	{ "legality_premodern", MKM_CONFIG_COLUMN_TYPE_SFC_STRING, SFC_CARD_STRING_LEGALITY_PREMODERN, MKM_FALSE, 0				},

	{ NULL, 0, 0, MKM_FALSE, 0																								}
};

static void
mkm_config_help()
{
	printf(
		"usage: mkmcsv [options] <input files>\n"
		"\n"
		"    Input files are read, processed into rows, optionally sorted, and then\n"
		"    output is generated.\n"
		"\n"
		"options:\n"
		"\n"
		"    --input <csv | shipments>\n"
		"        csv        Input files are CSV files exported by Cardmarket.\n"
		"        shipments  Each input file is a text file containing a list of\n"
		"                   shipments. For details run 'mkmcsv --help shipments'.\n"
		"\n"
		"    --output <text | csv>\n"
		"        text       Output is a text formated table of all data rows.\n"
		"        csv        Output is a CSV file.\n"
		"\n"
		"    --columns <list of columns>\n"
		"        Specifies the '+' seperated names of columns that should be shown in\n"
		"        the output. For a list of column names run 'mkmcsv --help columns'.\n"
		"\n"
		"    --sort <list of columns>\n"
		"        Sort output rows by the list of '+' seperated column names. Prefix\n"
		"        name with '-' for descending order instead of ascending.\n"
		"\n"
		"    --cache <path>\n"
		"        Sets the location of the scryfallcache database is stored. Defaults\n"
		"        to '.mkmcsv' in your home directory.\n"
		"\n"
		"    --verbose\n"
		"        Enables verbose output.\n"
		"\n"
		"    --output_file <path>\n"
		"        Writes output to specified file instead of stdout.\n"
		"\n"
		"    --whitelist_sets <list of sets>\n"
		"        Only include a row if the 'set' column matches one of the listed sets.\n"
		"\n"
		"    --blacklist_sets <list of sets>\n"
		"        Only include a row if the 'set' column doesn't match one of the listed set.\n"
		"\n"
		"    --config <path>\n"
		"        Loads options from a text file if it exists. Path defaults to\n"
		"        '.mkmcsv/config.txt' in your home directory.\n"
		"        One option per line in the text file, same as the ones on the command-\n"
		"        line, except without the leading '--'.\n"
	);
}

static void
mkm_config_help_columns()
{
	printf("List of column names accepted by --columns and --sort:\n");
	printf("\n");

	const mkm_config_column_info* p = g_mkm_config_column_info;
	while (p->name != NULL)
	{
		printf("    %s\n", p->name);
		p++;
	}
}

static void
mkm_config_help_shipments()
{
	printf("Description of the 'shipments' text file format:\n");
	printf("\n");

	printf(
		"A file contains a list of simple commands that are used to describe\n"
		"shipments you've received:\n"
		"\n"
		"csv <path template>\n"
		"    Tells mkmcsv where to find the CSV file associated with a\n"
		"    shipment, if any. '{}' is substituted with the shipment ID.\n"
		"\n"
		"    Example:\n"
		"        csv \"/some/path/some_csv_file_{}.csv\"\n"
		"\n"
		"    Defaults to \"ArticlesFromShipment{}..csv\"\n"
		"\n"
		"shipment <shipment ID> <date> <shipping cost> <trustee fee>\n"
		"    Creates a shipment with the specified ID. Date is in the form YYYYMMDD.\n"
		"    The path to the corresponding CSV file will be created using the\n"
		"    template as desribed above. If the CSV file doesn't exist, it will just\n"
		"    be ignored.\n"
		"\n"
		"    Example:\n"
		"        shipment 123456 20220413 4.00 0.20\n"
		"\n"
		"remove <set> <collector number> <condition>\n"
		"    Removes a card from the previously created shipment identified by its\n"
		"    set, collector number, and condition.\n"
		"\n"
		"    Example:\n"
		"        remove lea 287 EX\n"
		"\n"
		"        This will remove alpha card #287 in EX condition from the shipment.\n"
		"\n"
		"add <set> <collector number> <condition> <price>\n"
		"    Adds a card to the previously created shipment. Works similar to 'remove',\n"
		"    except that a price is specified.\n"
		"\n"
		"    Example:\n"
		"        add lea 287 EX 29.00\n"
		"\n"
		"adjust <overall price adjustment>\n"
		"    Adjusts the overall price of the shipment. The adjustment is spread out over\n"
		"    the cards depending on their cost.\n"
		"\n"
		"    Example:\n"
		"        adjust -1.00\n"
		"\n"
		"ignore_csv\n"
		"    The CSV file will be ignored even if it exists.\n"
		"\n"
		"abort\n"
		"    Terminates parsing the file. Rest of it is ignored.\n"
	);
}

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

static mkm_config_column*
mkm_config_get_column_by_name(
	mkm_config*						config,
	const char*						name)
{
	for(mkm_config_column* column = config->columns; column != NULL; column = column->next)
	{
		if(strcmp(column->info->name, name) == 0)
			return column;
	}

	return NULL;
}

static size_t
mkm_config_count_hidden_columns(
	const mkm_config*				config)
{
	size_t count = 0;

	for (mkm_config_column* column = config->columns; column != NULL; column = column->next)
	{
		if(column->hidden)
			count++;
	}

	return count;
}

static void
mkm_config_parse_columns(
	mkm_config*						config,
	mkm_config_column**				last_column,
	const char*						string,
	mkm_bool						hidden)
{
	char temp[1024];
	mkm_strcpy(temp, string, sizeof(temp));

	char* p = temp;

	for(;;)
	{
		size_t token_length = 0;
		while (p[token_length] != '+' && p[token_length] != '\0')
			token_length++;

		mkm_bool end_of_line = p[token_length] == '\0';

		p[token_length] = '\0';

		mkm_config_column* existing_column = mkm_config_get_column_by_name(config, p);

		if(existing_column == NULL)
		{
			/* Column not defined */
			mkm_config_column* column = MKM_NEW(mkm_config_column);

			column->info = mkm_config_find_column_info(p);
			column->hidden = hidden;
			
			if (*last_column != NULL)
				(*last_column)->next = column;
			else
				config->columns = column;

			*last_column = column;

			config->num_columns++;
		}
		else
		{
			/* Already defined - override hidden flag if needed */
			if(existing_column->hidden && !hidden)
				existing_column->hidden = MKM_FALSE;
		}

		if (end_of_line)
			break;

		p += token_length + 1;
	}
}

static void
mkm_config_parse_sort(
	mkm_config*						config,
	const char*						string)
{
	MKM_ERROR_CHECK(config->sort_columns == NULL, "Sorting columns specified more than once.");

	char temp[1024];
	mkm_strcpy(temp, string, sizeof(temp));

	char* p = temp;
	mkm_config_sort_column* last_sort_column = NULL;

	for(;;)
	{
		size_t token_length = 0;
		while (p[token_length] != '+' && p[token_length] != '\0')
			token_length++;

		mkm_bool end_of_line = p[token_length] == '\0';

		p[token_length] = '\0';

		int32_t order = 1;
		if(*p == '-')
		{
			order = -1;
			p++;
			token_length--;
		}

		const mkm_config_column_info* info = mkm_config_find_column_info(p);

		mkm_config_sort_column* sort_column = MKM_NEW(mkm_config_sort_column);

		sort_column->column_index = mkm_config_get_column_index_by_name(config, info->name);
		MKM_ERROR_CHECK(sort_column->column_index != UINT32_MAX, "Column not included, so can't be used for sorting: %s", info->name);

		sort_column->order = order;
			
		if (last_sort_column != NULL)
			last_sort_column->next = sort_column;
		else
			config->sort_columns = sort_column;

		last_sort_column = sort_column;

		if (end_of_line)
			break;

		p += token_length + 1;
	}
}

static void
mkm_config_parse_sets(
	mkm_config*		config,
	char*			string)
{
	MKM_ERROR_CHECK(config->set_filters == NULL, "Set filters specified more than once.");

	char* p = string;
	mkm_config_set_filter* last_set_filter = NULL;

	for (;;)
	{
		size_t token_length = 0;
		while (p[token_length] != '+' && p[token_length] != '\0')
			token_length++;

		mkm_bool end_of_line = p[token_length] == '\0';

		p[token_length] = '\0';

		int32_t order = 1;
		if (*p == '-')
		{
			order = -1;
			p++;
			token_length--;
		}

		mkm_config_set_filter* set_filter = MKM_NEW(mkm_config_set_filter);
		set_filter->set = mkm_strdup(p);

		if (last_set_filter != NULL)
			last_set_filter->next = set_filter;
		else
			config->set_filters = set_filter;

		last_set_filter = set_filter;

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

static void
mkm_config_get_default_path(
	const char*			file,
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
		mkm_strcpy(home_path, pw->pw_dir, sizeof(home_path));
	#endif

	char mkm_csv_dir[1024];
	{
		size_t result = snprintf(mkm_csv_dir, sizeof(mkm_csv_dir), "%s/.mkmcsv", home_path);
		MKM_ERROR_CHECK(result <= sizeof(mkm_csv_dir), "Path too long.");
	}

	mkm_config_make_dir(mkm_csv_dir);

	{
		size_t result = snprintf(buffer, buffer_size, "%s/%s", mkm_csv_dir, file);
		MKM_ERROR_CHECK(result <= buffer_size, "Path too long.");
	}
}

typedef struct _mkm_config_option 
{
	char*						value;
	mkm_bool					should_free_value;
	struct _mkm_config_option*	next;
} mkm_config_option;

typedef struct _mkm_config_option_list
{
	mkm_config_option*			head;
	mkm_config_option*			tail;
} mkm_config_option_list;

static mkm_bool
mkm_config_option_list_has_option(
	const mkm_config_option_list*	option_list,
	const char*						string)
{
	for(const mkm_config_option* option = option_list->head; option != NULL; option = option->next)
	{
		if(strcmp(string, option->value) == 0)
			return MKM_TRUE;
	}

	return MKM_FALSE;
}

static void
mkm_config_option_list_add(
	mkm_config_option_list*			option_list,
	char*							value,
	mkm_bool						should_free_value)
{
	mkm_config_option* option = MKM_NEW(mkm_config_option);
	option->value = value;
	option->should_free_value = should_free_value;

	if (option_list->tail != NULL)
		option_list->tail->next = option;
	else
		option_list->head = option;

	option_list->tail = option;
}

static void
mkm_config_option_list_init(
	mkm_config_option_list*		option_list,
	int							argc,
	char**						argv)
{
	if (argc == 1)
	{
		/* No arguments, just show help and exit */
		mkm_config_help();
		exit(0);
	}

	memset(option_list, 0, sizeof(mkm_config_option_list));

	char config_path[256];
	mkm_config_get_default_path("config.txt", config_path, sizeof(config_path));

	/* Add options from command line */
	for(int i = 1; i < argc; i++)
	{
		if(strcmp(argv[i], "--config") == 0)
		{
			i++;
			MKM_ERROR_CHECK(i < argc, "Expected path after --config.");
			mkm_strcpy(config_path, argv[i], sizeof(config_path));
		}
		else
		{
			mkm_config_option_list_add(option_list, argv[i], MKM_FALSE);
		}
	}

	/* Load config from file if it exists */
	{
		FILE* f = fopen(config_path, "r");
		if(f != NULL)
		{
			char line_buffer[1024];
			while(fgets(line_buffer, sizeof(line_buffer), f) != NULL)
			{
				/* Remove comments */
				{
					size_t len = strlen(line_buffer);
					for (size_t i = 0; i < len; i++)
					{
						if (line_buffer[i] == ';')
						{
							line_buffer[i] = '\0';
							break;
						}
					}
				}

				/* Parse */
				mkm_tokenize tokenize;
				mkm_tokenize_string(&tokenize, line_buffer);

				if(tokenize.num_tokens > 0)
				{
					char full_option_name[256];
					size_t required = (size_t)snprintf(full_option_name, sizeof(full_option_name), "--%s", tokenize.tokens[0]);
					MKM_ERROR_CHECK(required <= sizeof(full_option_name), "Buffer overflow.");

					/* Only add option from config file if it wasn't already supplied */
					if(!mkm_config_option_list_has_option(option_list, full_option_name))
					{
						mkm_config_option_list_add(option_list, mkm_strdup(full_option_name), MKM_TRUE);

						for(size_t i = 1; i < tokenize.num_tokens; i++)
							mkm_config_option_list_add(option_list, mkm_strdup(tokenize.tokens[i]), MKM_TRUE);
					}
				}
			}

			fclose(f);
		}
	}
}

static void
mkm_config_option_list_uninit(
	mkm_config_option_list*		option_list)
{
	mkm_config_option* option = option_list->head;
	while(option != NULL)
	{
		mkm_config_option* next = option->next;
		
		if(option->should_free_value)
			free(option->value);

		free(option);

		option = next;
	}
}

/*--------------------------------------------------------------------*/

void	
mkm_config_init(
	mkm_config*					config,
	int							argc,
	char**						argv)
{
	memset(config, 0, sizeof(mkm_config));

	config->input_callback = mkm_input_csv;
	config->output_callback = mkm_output_text;

	config->output_stream = stdout;

	mkm_config_option_list option_list;
	mkm_config_option_list_init(&option_list, argc, argv);

	/* Apply options */
	mkm_config_input_file* last_input_file = NULL;
	mkm_config_column* last_column = NULL;

	for(const mkm_config_option* option = option_list.head; option != NULL; option = option->next)
	{
		if(option->value[0] == '-')
		{
			if (strcmp(option->value, "--whitelist_sets") == 0)
			{
				option = option->next;
				MKM_ERROR_CHECK(option != NULL, "Expected set list after --whitelist_sets.");
				MKM_ERROR_CHECK((config->flags & MKM_CONFIG_SET_FILTER_BLACKLIST) == 0, "Can't use both --whitelist_sets and --blacklist_sets.");
				config->flags |= MKM_CONFIG_SET_FILTER_WHITELIST;
				mkm_config_parse_sets(config, option->value);
			}
			else if (strcmp(option->value, "--blacklist_sets") == 0)
			{
				option = option->next;
				MKM_ERROR_CHECK(option != NULL, "Expected set list after --blacklist_sets.");
				MKM_ERROR_CHECK((config->flags & MKM_CONFIG_SET_FILTER_WHITELIST) == 0, "Can't use both --whitelist_sets and --blacklist_sets.");
				config->flags |= MKM_CONFIG_SET_FILTER_BLACKLIST;
				mkm_config_parse_sets(config, option->value);				
			}
			else if(strcmp(option->value, "--output_file") == 0)
			{
				option = option->next;
				MKM_ERROR_CHECK(option != NULL, "Expected path after --output_file.");

				config->output_stream = fopen(option->value, "w");
				MKM_ERROR_CHECK(config->output_stream != NULL, "Failed to open file for output: %s", option->value);

				config->flags |= MKM_CONFIG_CLOSE_OUTPUT_STREAM_ON_EXIT;
			}
			else if(strcmp(option->value, "--columns") == 0)
			{
				option = option->next;
				MKM_ERROR_CHECK(option != NULL, "Expected columns after --columns.");
				mkm_config_parse_columns(config, &last_column, option->value, MKM_FALSE);
			}
			else if(strcmp(option->value, "--sort") == 0)
			{
				option = option->next;
				MKM_ERROR_CHECK(option != NULL, "Expected columns after --sort.");
				mkm_config_parse_sort(config, option->value);
			}
			else if (strcmp(option->value, "--cache") == 0)
			{
				option = option->next;
				MKM_ERROR_CHECK(option != NULL, "Expected path after --cache.");
				mkm_strcpy(config->cache_file, option->value, sizeof(config->cache_file));
			}
			else if(strcmp(option->value, "--input") == 0)
			{
				option = option->next;
				MKM_ERROR_CHECK(option != NULL, "Expected input type after --input.");
				const char* input_type = option->value;
				if(strcmp(input_type, "csv") == 0)
				{
					config->input_callback = mkm_input_csv;
				}
				else if (strcmp(input_type, "shipments") == 0)
				{
					config->input_callback = mkm_input_shipments;

					/* Add required columns for processing shipments */
					mkm_config_parse_columns(config, &last_column, "name+set+collector_number+version+condition+price+shipping_cost+trustee_fee", MKM_TRUE);
				}
				else
				{
					mkm_error("Invalid input type: %s", input_type);
				}
			}
			else if (strcmp(option->value, "--output") == 0)
			{
				option = option->next;
				MKM_ERROR_CHECK(option != NULL, "Expected output type after --output.");
				const char* output_type = option->value;
				if (strcmp(output_type, "text") == 0)
					config->output_callback = mkm_output_text;
				else if(strcmp(output_type, "csv") == 0)
					config->output_callback = mkm_output_csv;
				else
					mkm_error("Invalid output type: %s", output_type);
			}
			else if(strcmp(option->value, "--verbose") == 0)
			{
				config->flags |= MKM_CONFIG_VERBOSE;
			}
			else if(strcmp(option->value, "--help") == 0)
			{
				if (option->next != NULL && strcmp(option->next->value, "columns") == 0)
					mkm_config_help_columns();
				else if (option->next != NULL && strcmp(option->next->value, "shipments") == 0)
					mkm_config_help_shipments();
				else
					mkm_config_help();

				exit(0);
			}
			else
			{
				mkm_error("Invalid option: %s", option->value);
			}
		}
		else
		{
			/* Add input file */
			mkm_config_input_file* input_file = MKM_NEW(mkm_config_input_file);

			input_file->path = mkm_strdup(option->value);

			if(last_input_file != NULL)
				last_input_file->next = input_file;
			else 
				config->input_files = input_file;

			last_input_file = input_file;
		}
	}

	if(config->num_columns == mkm_config_count_hidden_columns(config))
	{
		/* All columns are hidden, add default columns ones */
		mkm_config_parse_columns(config, &last_column, "name+version+set+condition+price", MKM_FALSE);
	}

	if(config->cache_file[0] == '\0')
	{
		/* No cache file specified, determine default path */
		mkm_config_get_default_path("cache.bin", config->cache_file, sizeof(config->cache_file));
	}

	if(config->flags & MKM_CONFIG_VERBOSE)
	{
		for(const mkm_config_column* column = config->columns; column != NULL; column = column->next)
		{
			printf("Columm: %s %s\n", column->info->name, column->hidden ? "(hidden)" : "");
		}
	}

	mkm_config_option_list_uninit(&option_list);
}

void	
mkm_config_uninit(
	mkm_config*			config)
{
	if(config->flags & MKM_CONFIG_CLOSE_OUTPUT_STREAM_ON_EXIT)
	{
		fclose(config->output_stream);
	}

	/* Free input files */
	{
		mkm_config_input_file* input_file = config->input_files;
		while(input_file != NULL)
		{
			mkm_config_input_file* next = input_file->next;
			free(input_file->path);
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

	/* Free sorting keys */
	{
		mkm_config_sort_column* column = config->sort_columns;
		while (column != NULL)
		{
			mkm_config_sort_column* next = column->next;
			free(column);
			column = next;
		}
	}

	/* Free set filters */
	{
		mkm_config_set_filter* set_filter = config->set_filters;
		while (set_filter != NULL)
		{
			mkm_config_set_filter* next = set_filter->next;
			free(set_filter->set);
			free(set_filter);
			set_filter = next;
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
