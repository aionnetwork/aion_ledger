Introduction : We are directly sending transaction hash to Ledger Nano S as blue SDK doesn't support blake2b. Once user signs the hash, the signature is sent back. 

Note : Once the blue SDK start supporting blake2b, expect an update from us where we will be sending the RLP encoded transaction to Ledger Nano S for user to verify. Once user verifies it, the transaction hash will be generated on Nano S and signature will be sent back

HD Derivation path
-------------------
44'/425'/0'/0'/0'

Requirements
------------
JDK 10
Python 2.7
Python modules mentioned in 'PythonPackages.txt'

Test Scripts
------------
1) Navigate to 'test' directory
2) Enable 'TESTING_ENABLED' in Makefile
3) Modify following input in 'test_cases.py'
	- Fill the Nano S mnemonic values in 'phrase' field
4) Execute 'python execute_test_cases.py'

Installation Instructions
--------------------------
Install : make load BOLOS_SDK=SDK_LOCATION BOLOS_ENV=ENV_LOCATION
Deletion : make delete BOLOS_SDK=SDK_LOCATION BOLOS_ENV=ENV_LOCATION


