const size_t MAX_ERRORSTRING_LEN = 256;
const size_t MAX_ASSETSTRING_LEN = 64;
const size_t MAX_PRICESTRING_LEN = 32;

const short NO_BUY = 0;
const short HALF_BUY = 1;
const short FULL_BUY = 2;

const short BOUGHT = 0;
const short SOLD = 1;

struct InfoObject
{
  boolean error;                           //Was there an error that must be cleared?
  char errorString[MAX_ERRORSTRING_LEN];   //If there was an error, informative message here
  short lastTrade;                         //Did we buy or sell last? Do I want to consider half and half positions?
  double lastPrice;                        //What was the last price we traded at?
  short tradeStrategy;                     //Do we buy/sell with all of our money, half of it, or none of it?
  double reservedETH;                      //How much ETH should I never sell?
  int counter;
};

void readInfoObject(struct InfoObject* infoObjPtr);

void writeInfoObject(struct InfoObject* infoObjPtr);
