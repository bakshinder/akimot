#include <cxxtest/TestSuite.h>
#include "utils.h"
#include "board.h"
#include "hash.h"
#include "engine.h"

#define TEST_DIR "./test"
#define INIT_TEST_DIR "./test/init/"
#define INIT_TEST_LIST "./test/init/tests.txt"
#define HASH_TABLE_INSERTS 100

//mature level is defined in engine.h
#define UCT_TREE_TEST_MATURE 1 
#define UCT_TREE_NODES 100000
#define UCT_TREE_NODES_DELETE 100
#define UCT_TREE_MAX_CHILDREN 30

class DebugTestSuite : public CxxTest::TestSuite 
{
  public:


    /**
     * Utilities test - mostly string functions. 
     */
    void testUtils(void)
    {
      string s;

      s = " x ";
      TS_ASSERT_EQUALS(trimRight(trimLeft(s)), "x");

      s = "    x  ";
      TS_ASSERT_EQUALS(trimRight(trimLeft(s)), "x");
    }

    /**
     * Board init test.
     *
     * Inits two boards from game record file and position file
     * and tests their equality.
     */
     void testBoardInit(void)
     {
        Board board1;
        Board board2;
        fstream f;
        string fnPosition;
        string fnRecord;
        stringstream ss;
        string line;

        f.open(INIT_TEST_LIST, fstream::in);

        while (f.good()){
          getline(f, line);
          ss.clear();
          ss.str(line);
          ss >> fnPosition;
          ss >> fnRecord;
          fnPosition = INIT_TEST_DIR + fnPosition;
          fnRecord = INIT_TEST_DIR + fnRecord;

          board1.initFromPosition(fnPosition.c_str());
          board2.initFromRecord(fnRecord.c_str());

          TS_ASSERT_EQUALS( board1.getSignature(), board2.getSignature());
        }
     }

    /**
     * Hash table insert/hasItem/load test.
     */
    void testHashTable(void)
    {
      HashTable<int> hashTable; 
      u64 keys[HASH_TABLE_INSERTS];

      //store some random values into hash table
      for (int i = 0; i < HASH_TABLE_INSERTS; i++){
        keys[i] = getRandomU64();
        hashTable.insertItem(keys[i], i);
      }

      //positive-lookup test
      for (int i = 0; i < HASH_TABLE_INSERTS; i++){
        int item;
        TS_ASSERT_EQUALS(hashTable.hasItem(keys[i]), true); 
        if (! hashTable.loadItem(keys[i], item))
          TS_FAIL("positive-lookup failed");
      }
       
      //negative-lookup test
      for (int i = 0; i < HASH_TABLE_INSERTS; i++){
        int key = getRandomU64();
        int item;
        TS_ASSERT_EQUALS(hashTable.hasItem(key), false); 
        if (hashTable.loadItem(key, item))
          TS_FAIL("negative-lookup suceeded");
      }
    }

    /**
     * Uct test.
     *
     * Dummy version of Uct::doPlayout. Tests whether the uct tree is consistent 
     * after large number of inserts(UCT_TREE_NODES). Position in the bottom of the 
     * tree is evaluated randomly and randomDescend() is applied instead of uctDescend().  
     */
    void testUct(void)
    {
      //tree with random player in the root
      Tree* tree = new Tree(INDEX_TO_PLAYER(random() % 2));
      int winValue[2] = {1, -1};
      StepArray steps;
      int stepsNum;
      int i = 0;

      //build the tree in UCT manner
      for (int i = 0; i < UCT_TREE_MAX_CHILDREN; i++)
        steps[i] = Step();

      while (true) {
        tree->historyReset();     //point tree's actNode to the root 
        while (true) { 
          if (! tree->actNode()->hasChildren()) { 
            //if (tree->actNode()->getVisits() > UCT_TREE_TEST_MATURE) {
              stepsNum = (rand() % UCT_TREE_MAX_CHILDREN) + 1;
              tree->actNode()->expand(steps,stepsNum);
              i++;
              break;
            //}
            //tree->updateHistory(winValue[(random() % 2)]);
            //break;
          }
          tree->randomDescend(); 
          //tree->uctDescend(); 
        } 
        if (i >= UCT_TREE_NODES)
          break;
      }

      //remove given number of leafs
      for (int i = 0; i < UCT_TREE_NODES_DELETE; i++){
        tree->historyReset();
        while (tree->actNode()->hasChildren())
          tree->randomDescend(); 
        tree->actNode()->remove();
      }

      //TODO DFS through tree and check child.father == father, etc. ? 
    } //testUct

};
