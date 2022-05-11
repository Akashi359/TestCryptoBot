/*
 * Arbitrary number to compare to numTotal
 */
const short NUM_TRADE_THRESHOLD = 7;

/*
 * Get the amount of freeETH and freeUSD
 * Returns true on success, false on failure
 */
boolean getBalances(char* freeETH, const size_t freeETHLen, char* freeUSD, const size_t freeUSDLen);

/*
 * Get the amount of coins that went up by 5% in the last 24 hours,
 *     down by 5% in the last 24 hours,
 *     and the total number of coins counted.
 * Returns true on success, false on failure
 */
boolean getPriceChange(short* numPlusPtr, short* numMinusPtr, short* numTotalPtr);

/*
 * Get Fear and Greed index number.
 * Returns Fear and Greed index number on success, or -1 on failure.
 */
long getFNG();

/*
 * Get price of ETH.
 * Returns fall on failure, leaving *price unmodified.
 * Otherwise, returns true and puts the current price of eth in *price.
 */
boolean getETHPrice(char* priceBuffer, const size_t priceBufferLen);

/*
 * Buys ETH equivalent to the amount of USD listed
 */
boolean buy(char* quoteOrderQty, char* errorString, char* priceTradedAt, size_t priceTradedAtLen);

/*
 * Sells the amount of ETH listed
 */
boolean sell(char* quantity, char* errorString, char* priceTradedAt, size_t priceTradedAtLen);
