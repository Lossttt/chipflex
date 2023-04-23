#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// GNU/Linux and windows compatibility.
#ifdef _WIN32
	#define clrscr() system("cls");
#else
	#define clrscr() system("clear");
#endif

#include <json.h>

#define OPTIONS_FILE_NAME "options.json"
#define NAME_LEN 256

typedef struct option
{
	char name[NAME_LEN];
	uint64_t price;
} option;

typedef struct category
{
	char name[NAME_LEN];

	size_t option_count;
	option* options;
} category;

// Category functions.
void load_category_type(
	bool is_essential,
	struct json_object_s* root,
	size_t* out_category_count,
	category** out_categories
);

void load_categories(char* json_file_contents);

void print_category(category c);
void print_categories(char* category_type_name, size_t category_count, category* categories);

size_t request_choice_from_category(category c);

// Menu functions.
void start_menu();
void display_customer_types();
void display_all_options();
void generate_offer();

// Utility functions.
void wait_for_input();
char get_single_digit_input(int min, int max);
int get_y_n_input(bool default_value);

// Static variables.
static size_t essential_category_count;
static category* essential_categories;

static size_t extra_category_count;
static category* extra_categories;

int main()
{
	// Open file and get the file handle.
	FILE* fh = fopen(OPTIONS_FILE_NAME, "r");

	if (fh == NULL)
	{
		printf("Failed to open file `%s`.", OPTIONS_FILE_NAME);
		return 1;
	}

	// Get the size of the file in characters.
	// Seek the file position to the end of the file.
	fseek(fh, 0, SEEK_END);

	// Get the current position in the file, this will be equal to the file size because the position should now be at
	// the end of the file.
	size_t file_size = ftell(fh);

	// Reset the position back to the beginning of the file.
	rewind(fh);

	// Allocate enough memory to store the contents of the file.
	char* file_content = malloc(file_size + 1);

	// Read the file into the previously allocated memory.
	fread(file_content, 1, file_size, fh);
	file_content[file_size] = '\0';

	// Close the file handle as we no longer need it.
	fclose(fh);

	// Load the options from the json file.
	load_categories(file_content);

	// Start the menu.
	start_menu();

	// Free up allocations.
	free(file_content);
	return 0;
}

void load_category_type(
	bool is_essential,
	struct json_object_s* root,
	size_t* out_category_count,
	category** out_categories
)
{
	// Parse the essential options.
	struct json_array_s* e_categories =
		(struct json_array_s*) is_essential ? root->start->value->payload : root->start->next->value->payload;

	size_t e_category_count = e_categories->length;

	*out_category_count = e_category_count;
	*out_categories = malloc(e_category_count * sizeof(category));

	// Loop through categories.
	struct json_array_element_s* current_category = e_categories->start;
	for (size_t i = 0; i < e_category_count; i++)
	{
		// Get the category object.
		struct json_object_s* category_object = (struct json_object_s*) current_category->value->payload;

		// Get the category name.
		struct json_string_s* category_name = (struct json_string_s*) category_object->start->value->payload;

		// Copy over category name.
		memcpy(&(*out_categories)[i].name, category_name->string, category_name->string_size);
		(*out_categories)[i].name[category_name->string_size] = '\0'; // Add null-terminator

		// Get the array containing the options.
		struct json_array_s* category_options = (struct json_array_s*) category_object->start->next->value->payload;

		// Allocate enough memory to store all the options.
		(*out_categories)[i].option_count = category_options->length;
		(*out_categories)[i].options = malloc(category_options->length * sizeof(option));

		// Iterate the options and store them.
		struct json_array_element_s* current_option = category_options->start;
		for (size_t o = 0; o < category_options->length; o++)
		{
			// Get the option object.
			struct json_object_s* option_object = (struct json_object_s*) current_option->value->payload;

			// Get the name of the option.
			struct json_string_s* option_name = (struct json_string_s*) option_object->start->value->payload;

			// Copy over the object name.
			memcpy(&(*out_categories)[i].options[o].name, option_name->string, option_name->string_size);
			(*out_categories)[i].options[o].name[option_name->string_size] = '\0'; // Add null-terminator

			// Get the price of the option.
			struct json_number_s* option_price = (struct json_number_s*) option_object->start->next->value->payload;

			(*out_categories)[i].options[o].price = strtoull(option_price->number, NULL, 10);

			// Go to the next option before continueing the loop.
			current_option = current_option->next;
		}

		// Go to the next category before continueing the loop.
		current_category = current_category->next;
	}
}

void load_categories(char* json_file_contents)
{
	// Parse the options json.
	struct json_value_s* json = json_parse(json_file_contents, strlen(json_file_contents));
	
	struct json_object_s* root = (struct json_object_s*) json->payload;

	// Load the category types.
	load_category_type(true, root, &essential_category_count, &essential_categories);
	load_category_type(false, root, &extra_category_count, &extra_categories);

	free(json);
}

void print_category(category c)
{
	printf("%s\n", c.name);
		
	// Iterate and print the options.
	for (size_t o = 0; o < c.option_count; o++)
	{
		printf("\t%d. %-12s%-5d EUR\n", o, c.options[o].name, c.options[o].price);
	}

	printf("\n");
}

void print_categories(char* category_type_name, size_t category_count, category* categories)
{
	// Print the category type name.
	printf("---- %s ----\n\n", category_type_name);

	// Iterate and print the categories.
	for (size_t i = 0; i < category_count; i++)
	{
		print_category(categories[i]);
	}
}

size_t request_choice_from_category(category c)
{
	while (true)
	{
		// Clear the screen.
		clrscr();

		// Prompt.
		printf("Please choose an option from the following category:\n");
		print_category(c);
		printf("Input (0-%d): ", c.option_count - 1);

		char input = get_single_digit_input(0, c.option_count - 1);

		if (input == -1)
		{
			continue;
		}

		return input;
	}
}

void start_menu()
{
	char input = 0;

	while (input != 4)
	{
		clrscr();
		printf("Welcome to the ShipFlex offer assistant.\n\n");

		printf("Please choose from one of the options below.\n");
		printf("\t1. See customer types.\n\t2. Display all available options.\n\t3. Offer generator.\n\t4. Exit.\n");
		printf("Choice (1-4): ");

		// Get input.
		input = get_single_digit_input(1, 4);

		if (input == -1)
		{
			continue;
		}

		switch (input)
		{
		case 1:
			display_customer_types();
			break;
		case 2:
			display_all_options();
			break;
		case 3:
			generate_offer();
			break;
		case 4:
			break;
		}
	}
}

void display_customer_types()
{
	clrscr();
	printf("Customer types:\n\t1. Private.\n\t2. Company.\n\n");

	wait_for_input();
}

void display_all_options()
{
	clrscr();
	print_categories("Essential Categories", essential_category_count, essential_categories);
	print_categories("Extra Categories", extra_category_count, extra_categories);

	wait_for_input();
}

void generate_offer()
{
	clrscr();

	printf("Offer generator.\n\n");

	printf("Should offer be saved to a file?\n\nChoice (Y/n): ");


}

void wait_for_input()
{
	printf("Press [ENTER] to continue.");

	// Temporary buffer.
	char temp[8];

	fgets(temp, 8, stdin);
}

char get_single_digit_input(int min, int max)
{
	// Get input.
	char input_buffer[NAME_LEN];

	fgets(input_buffer, NAME_LEN, stdin);
	
	char input = input_buffer[0];

	if (input < '0' || input > '9')
	{
		printf("\nThe input '%c' is not a numeric character. Press enter to retry.");
		wait_for_input();
		return -1;
	}

	input = input - '0'; // Get the actual numeric value.

	if (input > max || input < min)
	{
		printf(
			"\nThe input '%d' is out of bounds for the given category. Please choose a number between %d and %d.",
			input,
			min,
			max
		);
		wait_for_input();
		return -1;
	}

	return input;
}

int get_y_n_input(bool default_value)
{
	// Get input.
	char input_buffer[NAME_LEN];

	fgets(input_buffer, NAME_LEN, stdin);
	
	char input = input_buffer[0];

	// Is input empty? 
	if (input == '\0')
	{
		return default_value;
	}

	if (input == 'y' || input == 'Y')
	{
		return true;
	}

	if (input == 'n' || input == 'N')
	{
		return false;
	}

	// No valid input was given.
	printf("\nProvided unrecognized input. Please try again.");
	wait_for_input();
	return -1;
}
