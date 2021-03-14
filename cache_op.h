#pragma once
#include <iostream>
#include<vector>

namespace cache_sim
{
    /*cache level 
    */
   // int op_times=0 ;
    enum cache_level
    {
        CACHE_L1 = 0,
        CACHE_L2 = 1,
        CACHE_L3 = 2,
        CACHE_MAX = 255,
        CACHE_MAIN = CACHE_MAX
    };

    enum method_to_find_free_block
    {
        METHOD_LRU,
        METHOD_PLRU,
        METHOD_OPTIMAL
    };

    /*define a mem operation*/
    typedef struct tag_mem_op {
        char op_code;                                          //read or write operations
        unsigned int op_addr;   //ox 7b0348fc= 32-bit address
        int op_times; // operation count
    }mem_op_st;     
                                           // a struct type obj:mem_op_st, for trace files


    /*cache config structure
    ref::http://www.wowotech.net/memory_management/458.html
    */

    typedef struct tag_cache_config {
        int cache_size;
        int assoc_num;
        int sets_num;
        int len_tag_bits;
        int len_index_bits;
        int len_blockoffset_bits;
        int method_to_find_victim_block;
        int cache_level;                                         /* default 0 means L1*/
        int block_size;
        int inclusion_property;

        tag_cache_config()                                      //constructor by overloading
        {
            memset(this, 0, sizeof(tag_cache_config));          //this (pointer) points to the first bytes in this struct
        }                                                       //http://www.cplusplus.com/reference/cstring/memset/

        tag_cache_config(                                       //constructor by overloading, used for initialization
            int _cache_size,
            int _assoc,                                         //n-way associative
            int _len_sets,
            int _len_tag_bits,
            int _len_index_bits,
            int _len_blockoffset_bits,
            int _cache_level):
            cache_size(_cache_size),
            assoc_num(_assoc),
            sets_num(_len_sets),
            len_tag_bits(_len_tag_bits),
            len_index_bits(_len_index_bits),
            len_blockoffset_bits(_len_blockoffset_bits),
            cache_level(_cache_level),
            block_size(0),
            inclusion_property(0)
        {

        }
    }cache_config_st;                                           // a struct type obj： cache_config_st
 
    /*block detail */
    typedef struct tag_cache_block {
        bool valid;
        bool dirty;
        int lru_times;
        int opt_times;
        unsigned int tag;
        unsigned int memblock_addr;
        mem_op_st hex_addr;                                    // store original hexAddress from tracefile
        tag_cache_block()                                      //constructor by overloading
        {
            memset(this, 0, sizeof(tag_cache_block));          //this (pointer) points to the first bytes in this struct
        }                                                      //http://www.cplusplus.com/reference/cstring/memset/
    }cache_block_st;

    /* blocks */
    typedef struct tag_cache_set {
        int assoc;
        int current_max_rlu=0;
        int current_max_opt=0;
        unsigned int prlu_flag;
        cache_block_st *p_blocks;                             // a struct pointer: call members in struct tag_cache_block
        tag_cache_set(int _assoc = 0):                        //constructor by overloading, used for initializing assoc,prlu_flag
            assoc(_assoc),
            p_blocks(NULL),
            prlu_flag(0xff)
        {
        }
    }cache_set_st;                                           // a struct type obj： cache_set_st

    typedef struct tag_cache_op_sta
    {
        int read_times;
        int read_missed_times;
        int write_times;
        int write_missed_times;
        int writeback_times;
        int writeback_times1;//L1 write to Main Mem,not L2 when inclusive L2 evict B, L1 also evict B, if B is dirty in L1, then wirte to Main Mem,not L2

        tag_cache_op_sta():
            read_times(0),
            read_missed_times(0),
            write_times(0),
            write_missed_times(0),
            writeback_times(0),
            writeback_times1(0)
        {
        }
    }cache_op_sta_st;

    /*cache info
    cfg means config info or parameters of this cache
    sta means all kind statistic detail of this cache itself, NOT INCLUDE ANY OTHER LEVEL CACHES
    next_catche means next level cache, or nullptr 
    */
    typedef struct tag_cache_info {
        cache_config_st cfg;
        cache_set_st *sets;
        cache_op_sta_st sta;
        tag_cache_info* next_catche;
        tag_cache_info() :
            sets(nullptr),
            next_catche(nullptr)
        {
        }
    }cache_info_st;   // this struct obj can accesss all members defined above by a pointer

    class CacheOperate
    {
    public:
        CacheOperate();
        ~CacheOperate();
        
        //static int Sendaddr(std::vector<int> &alladdr);
       

    private:
        /*************************************************
        Function:        retrieve_block_info
        Description:   retrieve block tag, index, mem addr from addr
        Input:             int op_addr : input addr
                              const cache_config_st& cache_cfg cache : config params
                              int& tag :tag 
                              int& index:index
                              int& mem_addr_by_block:mem addr by block, point to the 1st bit addr of block
        Output:          none
        Return:          true or  false 
        Others:          none
        *************************************************/
        static bool retrieve_block_info(int op_addr,
            const cache_config_st& cache_cfg,
            int& tag, int& index, int& mem_addr_by_block);

        /*************************************************
        Function:        retrieve_victim_block
        Description:    find a victim block,this means can not find any empty block. we have to find an victim block
		                       be care about the read,write and sublevel cache operations.
        Input:             cache_info_st* cache :cache params
                              int mothod :the way how to find an victim
							   int set_idx: set index
                              int& block_index : return (out) the block index we found   (set,offset ?)
        Output:         none
        Return:          true  or false
        Others:          
        *************************************************/
        static bool retrieve_victim_block(cache_info_st* cache, int mothod, int set_idx, int& block_index);

        /*************************************************
        Function:       retrieve_victim_block_by_RLU
        Description:   find an victim by RLU
        Input:             cache_info_st* cache cache  params
		                       int set_idx: set index
                              int& block_index :return (out) block index
        Output:          none
        Return:          true or false
        Others:          none
        *************************************************/
        static bool retrieve_victim_block_by_RLU(cache_info_st* cache,int set_idx, int& block_index);

		/*************************************************
		Function:       retrieve_victim_block_by_PRLU
		Description:   find an victim by PRLU
		Input:             cache_info_st* cache cache  params
		                       int set_idx: set index
							  int& block_index :return (out) block index
		Output:          none
		Return:          true or false
		Others:          none
		*************************************************/
        static bool retrieve_victim_block_by_PRLU(cache_info_st* cache, int set_idx, int& block_index);
        static bool retrieve_victim_block_by_OPT(cache_info_st* cache, int set_idx, int& block_index);
		/*************************************************
		Function:       retrieve_empty_block
		Description:   find an empty
		Input:             cache_info_st* cache cache  params
							  int set_idx: set index
							  int& block_index :return (out) block index
		Output:          none
		Return:          true or false
		Others:          none
		*************************************************/
        static bool retrieve_empty_block(cache_info_st* cache, int set_idx, int& block_index);

        /*************************************************
        Function:        op_write_back
        Description:    write operation 
        Input:             cache_info_st* cache cache params
                              int op_addr  op addr
        Output:          none
        Return:          true or  false
        Others:          none
        *************************************************/
        static bool op_write_back(cache_info_st* cache, int op_addr);

		/*update prlu flag while accessing blocks*/
		static bool update_prlu_flag(cache_info_st* cache,int block_index, int acess_block_index);

    public:
		/*************************************************
		Function:        op_mem_data
		Description:   cache write op   (write mem)
		Input:             cache_info_st* cache cache params
		                      char op_type
							  int op_addr
		Output:          none
		Return:          true or false
		Others:          none
		*************************************************/
        //static bool op_mem_data(cache_info_st* cache, char op_type, int op_addr);
        static bool op_mem_data(cache_info_st* cache, char op_type, int op_addr, int op_times);
		/*************************************************
		Function:        op_write_data
		Description:   cache write op  (write cache block)
		Input:             cache_info_st* cache cache params
							  int op_addr op address
							  cache_block_st& op_block  the block we write to
		Output:          none
		Return:          true or false
		Others:          none
		*************************************************/
        static bool op_write_data(cache_info_st* cache, int op_addr, cache_block_st& op_block);

        /*************************************************
        Function:        op_read_data
        Description:   cache read op         (read cache block)
        Input:             cache_info_st* cache cache params
                              int op_addr op address
                              cache_block_st& op_block  the block we read from
        Output:          none
        Return:          true or false
        Others:          none
        *************************************************/
        static bool op_read_data(cache_info_st* cache, int op_addr, cache_block_st& op_block);

    public:
        static std::vector<int> alladdr;
        static int op_times1;// operation count
        static int opt_count ;
    };
}

