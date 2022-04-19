# mkmcsv
[![CMake](https://github.com/demogorgon1/mkmcsv/actions/workflows/cmake.yml/badge.svg)](https://github.com/demogorgon1/mkmcsv/actions/workflows/cmake.yml)

Cardmarket allows you to export CSV files listing your purchases, but unfortunately these contain somewhat limited information. This command-line utility will read these CSV files, process them, and then generate new files that are more useful. For example, it can find card names, types, and recent prices, and add them to the output. 

Several output modes are supported:

- CSV: Similar to the input files exported from Cardmarket, but with whatever additional columns you desire.
- Text: An easy-to-read text table.
- SQL: You can feed this directly into the SQL database you're using to manage your Magic cards. 

In addition to processing CSV files, mkmcsv can also read higher level shipment lists written by you to provide more information about your purchases, like shipping costs, trustee fees, and refunds. You can also use shipment lists to describe purchases from outside Cardmarket. The goal is to make the process of managing your new purchases as easy and steamlined as possible.

Information about cards are acquired through the public Scryfall API using the [scryfallcache](https://github.com/demogorgon1/scryfallcache) library.

## Installing
### From source
First of all you'll need git and cmake to acquire and build the project. Then you can run:
```
git clone https://github.com/demogorgon1/mkmcsv.git
cd mkmcsv
mkdir build
cd build
cmake ..
cmake --build .
```
On Windows, if successful, you'll find ```mkmcsv.exe``` in ```src\Debug```.
On Linux (and similar), if successful, you can run ```make install``` to install ```mkmcsv```.

## Usage
### Basic CSV processing
So, you've exported a CSV file (```ArticlesFromShipment123456..csv```) from Cardmarket and it looks something like this:
```
idProduct;groupCount;price;idLanguage;condition;isFoil;isSigned;isAltered;isPlayset;isReverseHolo;isFirstEd;isFullArt;isUberRare;isWithDie
5942;1;3.14;1;1;;;;;;;;;
6024;1;1.29;1;1;;;;;;;;;
```
```mkmcsv ArticlesFromShipment123456..csv``` reads the above and outputs the following to stdout:
```
name          |version|set|condition|price|
--------------|-------|---|---------|-----|
Time Elemental|      0|4ed|        1| 3.14|
         Swamp|      0|4ed|        1| 1.29|
```
You can use the ```--columns``` option to get other columns than the five default ones shown above. For example,
```mkmcsv ArticlesFromShipment123456..csv --columns name+set_name+artist``` will produce the following output instead:
```
name          |set_name      |artist     |
--------------|--------------|-----------|
Time Elemental|Fourth Edition|  Amy Weber|
         Swamp|Fourth Edition|Dan Frazier|
```
For a full list of available columns run ```mkmcsv --help columns```.

The example above used the default ```text``` output mode. To produce CSV output instead run ```mkmcsv ArticlesFromShipment123456..csv --output csv```.
This will produce the output:
```
name;version;set;condition;price
Time Elemental;0;4ed;1;314
Swamp;0;4ed;1;129
```

### Shipment lists
Instead of passing CSV files directly to the program, you can also write a shipment list file. In this file you should give some additional details about the shipment, which will produce additional information in the output. Let's say you write the following into a text file and saves it as ```shipments.txt```:
```
shipment 123456 20210720 2.00 0.20
```
This tells the program to include shipment with purchase id ```123456``` which was ordered on 2021-07-20 with a shipping cost of 2.00 and a trustee fee of 0.20. If the associated CSV file is found (named ```ArticlesFromShipment{purchase id}..csv```) it will be loaded and added to the shipment. Run ```mkmcsv shipments.txt --input shipments --columns name+price+shipping_cost+trustee_fee``` to produce the following output:
```
name          |price|shipping_cost|trustee_fee|
--------------|-----|-------------|-----------|
Time Elemental| 3.14|         1.42|       0.15|
         Swamp| 1.29|         0.58|       0.05|
```
The shipping cost and trustee fee columns are weighted based on the price of the card compared to the total cost of all cards in the shipment. Any rounding error is added to the most expensive card.
You can add multiple ```shipment``` statements to the same shipment file, which will cause all of them to be included in the output. For the sake of simplicity, let's say you have another shipment (654321) that includes the exact same cards as the other one:
```
shipment 123456 20210720 2.00 0.20
shipment 654321 20210720 2.00 0.20
```
Run ```mkmcsv shipments.txt --input shipments --columns name+price+shipping_cost+trustee_fee+purchase_id```, which will output:
```
name          |price|shipping_cost|trustee_fee|purchase_id|
--------------|-----|-------------|-----------|-----------|
Time Elemental| 3.14|         1.42|       0.15|     123456|
         Swamp| 1.29|         0.58|       0.05|     123456|
Time Elemental| 3.14|         1.42|       0.15|     654321|
         Swamp| 1.29|         0.58|       0.05|     654321|
```
Notice the added ```purchase_id``` column.

If desired, you can sort the output with ```--sort``` followed by a ```+```-separated list of colums to sort by, ordered by priority. Prefix a column name with ```-``` to change the order.

For more information about statements availabe you can use in shipment lists run ```mkmcsv --help shipments```.

### SQL
With ```--output sql``` you can convert your shipment lists into a bunch of SQL statements that can be used to update a postgres database. Use the ```--sql_table``` option to specify the name of the database table. The ```unique_id``` column (shipment id and CSV row number) will be added automatically as it is required as primary key in the table.. Run ```mkmcsv shipments.txt --input shipments --output sql --columns name+price```:
```
CREATE TABLE IF NOT EXISTS cards (
unique_id VARCHAR(64) PRIMARY KEY,
name TEXT ,
price INT );
INSERT INTO cards (unique_id, name, price)
VALUES('1E240-1', 'Time Elemental', 314)
ON CONFLICT (unique_id) DO UPDATE SET
name = EXCLUDED.name,
price = EXCLUDED.price;
INSERT INTO cards (unique_id, name, price)
VALUES('1E240-2', 'Swamp', 129)
ON CONFLICT (unique_id) DO UPDATE SET
name = EXCLUDED.name,
price = EXCLUDED.price;
INSERT INTO cards (unique_id, name, price)
VALUES('9FBF1-1', 'Time Elemental', 314)
ON CONFLICT (unique_id) DO UPDATE SET
name = EXCLUDED.name,
price = EXCLUDED.price;
INSERT INTO cards (unique_id, name, price)
VALUES('9FBF1-2', 'Swamp', 129)
ON CONFLICT (unique_id) DO UPDATE SET
name = EXCLUDED.name,
price = EXCLUDED.price;
```
This will create the table if it doesn't exist and try to insert the rows. If a row with ```unique_id``` already exists its values will be updated.

### Managing non-Cardmarket purchases
If you bought some cards on ebay (or wherever) you can easily add them to your shipment lists as well, you just need to come up with your own purchase ids that don't match any CSV files you exported from Cardmarket. Any numbers less than 10000'ish should be safe to use. As an example, consider the following shipment list file:
```
shipment 1 20220101 1.00 0.50
add 3ed 200 EX 2
```
The ```add``` statement adds 200th (collector's number) card from the set ```3ed``` (Revised Edition) in EX condition to the shipment with a price of 2.00. You can use the [Scryfall website](https://scryfall.com/) to look up collector's numbers and set codes.

### Modifying shipments
The ```add``` statement can also be used to add extra cards that were received in a shipment through Cardmarket. Missing cards can be removed with the ```remove``` statement:
```
remove 3ed 200 EX
```
This will find a card, same princible as with ```add```, but will remove it from the shipment. 
If a seller has offered a refund for something, but you're not sure what it was, you can use the ```adjust``` statement:
```
adjust -1.00
```
This will decrease the total overall price of the shipment by 1.00. An amount is deducted from each card in the shipment based on their contribution to the total cost. Any rounding error is deducted from the most expensive card.
