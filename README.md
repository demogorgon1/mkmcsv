# mkmcsv
[![CMake](https://github.com/demogorgon1/mkmcsv/actions/workflows/cmake.yml/badge.svg)](https://github.com/demogorgon1/mkmcsv/actions/workflows/cmake.yml)

Cardmarket allows you to export CSV files listing your purchases, but unfortunately these contain somewhat limited information. This command-line utility will read these CSV files, process them, and then generate new files that are more useful. For example, it can find card names, types, and recent prices, and add them to the output. 

Several output modes are supported:

- CSV: Similar to the input files exported from Cardmarket, but with whatever additional columns you desire.
- Text: An easy-to-read text table.
- SQL: You can feed this directly into the SQL database you're using to manage your Magic cards. (Not implemented yet.)

In addition to processing CSV files, mkmcsv can also read higher level shipment lists you can write to provide more information about your purchases, like shipping costs, trustee fees, and refunds. You can also use shipment lists to describe purchases from outside Cardmarket.

Information about cards are acquired through the public Scryfall API using the [scryfallcache](https://github.com/demogorgon1/scryfallcache) library.
