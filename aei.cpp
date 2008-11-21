#include "aei.h"


//--------------------------------------------------------------------- 
// section AeiRecord
//--------------------------------------------------------------------- 

AeiRecord::AeiRecord(string command, aeiState_e state, aeiState_e nextState, aeiAction_e action):
  state_(state), command_(command), nextState_(nextState), action_(action)
{
}

//--------------------------------------------------------------------- 
// section Aei
//--------------------------------------------------------------------- 

Aei::Aei()
{
  records_.clear();
  records_.push_back(AeiRecord(STR_AEI, AS_OPEN, AS_MAIN, AA_OPEN));
  records_.push_back(AeiRecord(STR_READY, AS_ALL, AS_SAME, AA_READY));
  records_.push_back(AeiRecord(STR_QUIT, AS_ALL, AS_SAME, AA_QUIT));
  records_.push_back(AeiRecord(STR_NEW_GAME, AS_MAIN, AS_GAME, AA_NEW_GAME));
  records_.push_back(AeiRecord(STR_SET_POSITION, AS_GAME, AS_SAME, AA_SET_POSITION));
  records_.push_back(AeiRecord(STR_SET_POSITION_FILE, AS_GAME, AS_SAME, AA_SET_POSITION_FILE));
  records_.push_back(AeiRecord(STR_SET_OPTION, AS_MAIN, AS_SAME, AA_SET_OPTION));
  records_.push_back(AeiRecord(STR_GO, AS_GAME, AS_SEARCH, AA_GO));
  records_.push_back(AeiRecord(STR_STOP, AS_SEARCH, AS_GAME, AA_STOP));

  timeControls_.push_back(timeControlPair(STR_TC_MOVE, TC_MOVE));
  timeControls_.push_back(timeControlPair(STR_TC_RESERVE, TC_RESERVE));
  timeControls_.push_back(timeControlPair(STR_TC_PERCENT, TC_PERCENT));
  timeControls_.push_back(timeControlPair(STR_TC_MAX, TC_MAX));
  timeControls_.push_back(timeControlPair(STR_TC_TOTAL, TC_TOTAL));
  timeControls_.push_back(timeControlPair(STR_TC_TURNS, TC_TURNS));
  timeControls_.push_back(timeControlPair(STR_TC_TURN_TIME, TC_TURN_TIME));
  timeControls_.push_back(timeControlPair(STR_TC_W_RESERVE, TC_W_RESERVE));
  timeControls_.push_back(timeControlPair(STR_TC_B_RESERVE, TC_B_RESERVE));
  timeControls_.push_back(timeControlPair(STR_TC_W_USED, TC_W_USED));
  timeControls_.push_back(timeControlPair(STR_TC_B_USED, TC_B_USED));
  timeControls_.push_back(timeControlPair(STR_TC_LAST_MOVE_USED, TC_LAST_MOVE_USED));
  timeControls_.push_back(timeControlPair(STR_TC_MOVE_USED, TC_MOVE_USED));

  state_ = AS_OPEN;
  board_ = new Board();
  engine_ = new Engine();
}

//--------------------------------------------------------------------- 

Aei::~Aei()
{
  delete board_;
  delete engine_;
}

//--------------------------------------------------------------------- 

void Aei::runLoop() 
{
  string line;
  while (true) {
    if (cin.good()){
      getline(cin, line);
      if (line != "")
        handleInput(line);
    }
  }
}

//--------------------------------------------------------------------- 

void Aei::handleInput(const string& line)
{
  stringstream ssLine;
  stringstream rest;
  ssLine.str("");
  ssLine.str(line);
  string command = "";
  ssLine >> command; 
  response_ = "";
  
  AeiRecord* record = NULL;
  AeiRecordList::iterator it; 

  for (it = records_.begin(); it != records_.end(); it++)
    if ((it->state_ == state_ || it->state_ == AS_ALL) && it->command_ == command){
      record = &(*it); 
      break;
    }

  if (! record) {
    response_ = STR_INVALID_COMMAND;
    quit();
  }

  if (record->nextState_ != AS_SAME)
    state_ = record->nextState_;

  aeiAction_e action = record->action_;
  switch (action) {
    case AA_OPEN:
                  sendId();
                  response_ = STR_AEI_OK;
                  break;
    case AA_READY: 
                  response_ = STR_READY_OK;
                  break;
    case AA_NEW_GAME: 
                  break;
    case AA_SET_OPTION:
                  handleOption(getStreamRest(ssLine));
                  break;
    case AA_SET_POSITION_FILE: 
                  board_->initFromPosition(getStreamRest(ssLine).c_str());
                  break;
    case AA_SET_POSITION: 
                  rest.str(getStreamRest(ssLine));
                  board_->initFromPositionStream(rest);
                  break;
    case AA_GO:    
                  startSearch(getStreamRest(ssLine));
                  break;
    case AA_STOP: 
                  engine_->requestSearchStop();
                  break;
    case AA_QUIT: 
                  response_ = STR_BYE;
                  quit();
                  break;
    default: 
             assert(false);
             break;
  }

  send(response_);
}

//--------------------------------------------------------------------- 

void Aei::handleOption(const string& commandRest)
{
  string option;
  string value;
  stringstream ss ;
  ss.str(commandRest);

  ss >> option;
  ss >> value;

  TimeControlList::iterator it;
  for (it = timeControls_.begin(); it != timeControls_.end(); it++)
    if (it->first == option){
      engine_->timeManager()->setTimeControl(it->second, str2int(value));
      return;
    }
}

//--------------------------------------------------------------------- 

void Aei::startSearch(const string& arg)
{
  int rc;

  if (arg == STR_PONDER || arg == STR_INFINITE)
    engine_->timeManager()->setNoTimeLimit();
  //no mutex is needed - this is done only when no engineThread runs
  rc = pthread_create(&engineThread, NULL, Aei::SearchInThreadWrapper, this);

  if (rc) //allocating thread failed
    quit();
}

//--------------------------------------------------------------------- 

void Aei::implicitSessionStart()
{
  handleInput(STR_AEI);
  handleInput(STR_NEW_GAME);
  handleInput((string) STR_SET_POSITION_FILE + " " + config.fnInput());
  handleInput(STR_GO);
}

//--------------------------------------------------------------------- 

void Aei::searchInThread()
{
  engine_->doSearch(board_);
  state_ = AS_GAME; //TODO possible ?  
  response_ = string(STR_BEST_MOVE) + " " + engine_->getBestMove();
  send(response_);
}

//--------------------------------------------------------------------- 

void * Aei::SearchInThreadWrapper(void* instance)
{
  Aei* aei= (Aei*) instance;
  aei->searchInThread();
  return NULL;
}

//--------------------------------------------------------------------- 

void Aei::quit() const
{
  send(response_);
  exit(1);
}

//--------------------------------------------------------------------- 

void Aei::sendId() const
{
  stringstream ss; 
  ss.clear();

  ss << STR_NAME << " " << ID_NAME;
  send(ss.str());
  ss.str("");

  ss << STR_AUTHOR << " " << ID_AUTHOR;
  send(ss.str());
  ss.str("");

  ss << STR_VERSION << " " << ID_VERSION;
  send(ss.str());
  ss.str("");
}

//---------------------------------------------------------------------

void Aei::send(const string& s) const
{
  if (s.length())
  cout << s << endl;
}

//--------------------------------------------------------------------- 
//--------------------------------------------------------------------- 
