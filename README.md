# Teradata UDFs

Collection of useful user-defined functions (UDF) to use in Teradata.

### NIFVALIDO

This UDF verifies and normalizes Spanish fiscal ID numbers.

- ```INPUT : varchar(20)```
- ```OUTPUT: varchar(20)``` -- with a valid ID ```CHAR(9)```

Operations:

- remove non-alphanumeric chars
- change to capital letters
- add non-significant leading zeros
- add control digit when missing (DNI/NIE, not legal entities)
- verify control digit when received
- return an empty string when not a valid NIF

Remarks: only personal IDs (not legal entities) have been tested

The code includes the BTEQ to create the new function in Teradata,
and the source code in ```.c```
