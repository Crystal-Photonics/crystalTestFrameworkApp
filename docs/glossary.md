
\section GLOSSARY_spectrum Spectrum
We use the term spectrum to describe an energy distribution of a radioactive isotope.
It is a histogram with the energy[unit: eV,KeV or MeV] as x-axis and the number of radioactive event-occurrences as y-axis.

\section GLOSSARY_json JSON File
The Crystal Test Framework uses often JSON files to store or load values from/to the hard disk. Especially \ref configuration or
database files are structured in the JSON format. A very important JSON file is the desired_value_database of the Data_engine. JSON files are
normal text files which are both: fairly human readable and machine readable. JSON files contain number, text bool and array elements.
Each element has a name and a value.

\par example:
\code
{
	"textelement": "Hello World",
	"numberelement": 13.456,
	"boolelement": true,
	"array_element1": [
		{
			"firstarray_element_bool": false
		},
		{
			"secondarray_element_bool": true
		}
	],
	"array_element2": [1, 2, 3, 4]
}
\endcode

If you need more assistance to understand the JSON file format click here:<a href=" https://en.wikipedia.org/wiki/JSON"> https://en.wikipedia.org/wiki/JSON</a>
It is highly recommended to check the syntax of the file each time you have edited the file via online tools like <a href="https://jsonlint.com/">
    https://jsonlint.com/</a> to be sure that it does not contain errors.
