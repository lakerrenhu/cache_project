#include "cache_op.h"
#include<vector>

namespace cache_sim
{
    
    //std::vector<int> alladdr;

    int CacheOperate::op_times1 = 0;
    int CacheOperate::opt_count = 0;
    std::vector<int> CacheOperate::alladdr = std::vector<int>();
    CacheOperate::CacheOperate()
    {

    }

    CacheOperate::~CacheOperate()
    {

    }
    /*
    for 32 bits
    |----TAG-----||----index----||----blockoffset----|
    XXXX XXXXXXXXXXXXXXXXXXXXXXXX
    ref:http://www.wowotech.net/memory_management/458.html
    */
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
    

    bool CacheOperate::retrieve_block_info(int op_addr,
        const cache_config_st& cache_cfg,
        int& tag, int& index, int& mem_addr_by_block)
    {                       //eg:offset=4,op_addr=0x400321ec, >>4 is ox0400321e, <<4 is ox4003210e as block addr

        mem_addr_by_block = (op_addr >> cache_cfg.len_blockoffset_bits) << cache_cfg.len_blockoffset_bits;
        index = (op_addr >> cache_cfg.len_blockoffset_bits) & ((1 << cache_cfg.len_index_bits) - 1);
        tag = (op_addr >> (cache_cfg.len_blockoffset_bits + cache_cfg.len_index_bits));

        return true;
    }

    bool CacheOperate::retrieve_victim_block(cache_info_st* cache, int mothod, int set_idx, int& block_index)
    {
        bool b = false;
        switch (mothod)
        {
        case METHOD_LRU:
            b = retrieve_victim_block_by_RLU(cache, set_idx, block_index);
            break;
        case METHOD_PLRU:
            b = retrieve_victim_block_by_RLU(cache, set_idx, block_index);
            break;
        case METHOD_OPTIMAL:
            b = retrieve_victim_block_by_OPT(cache, set_idx, block_index);
            break;
        }
        return b;
    }

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
    bool CacheOperate::retrieve_victim_block_by_RLU(cache_info_st* cache, int set_idx, int& block_index)
    {
        if (nullptr == cache
            || set_idx >= cache->cfg.sets_num)                 // empty pointer or violate the index limit,false
        {
            return false;
        }
        int rlu_max = INT_MAX;                                //default int max
        for (int i = 0; i < cache->cfg.assoc_num; i++)        //exhaust all ways (columns) at a set_idx index (row) 
        {   //cache(pointer) calls sets[], sets[] calls p_block[], p_block[] calls lru_times
            //if (rlu_max>lru_times and valid block), find the min lru_times and its block_index by loop

            if ((rlu_max > cache->sets[set_idx].p_blocks[i].lru_times)
                && cache->sets[set_idx].p_blocks[i].valid)
            {
                rlu_max = cache->sets[set_idx].p_blocks[i].lru_times;
                block_index = i;
            }
        }
        return true;
    }

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
    bool CacheOperate::retrieve_victim_block_by_PRLU(cache_info_st* cache, int set_idx, int& block_index)
    {
        if (nullptr == cache)
        {
            return false;
        }
        int prlu_flag_valid_bit_len = cache->cfg.assoc_num - 1;//(n-assoc)-1,eg: ABCD needs 3 binary nodes to build a tree
        if (prlu_flag_valid_bit_len <= 0)
        {
            block_index = 0;
        }
        int tmp_block_idx = 0, tmp_prlu_flag = cache->sets[set_idx].prlu_flag;
        for (int i = prlu_flag_valid_bit_len; i > 0; i--)                       //binary tree 
        {
            tmp_block_idx |= (tmp_prlu_flag & (1 << (i - 1)));
            tmp_prlu_flag ^= (1 << (i - 1));
        }
        block_index = tmp_block_idx;
        cache->sets[set_idx].prlu_flag = tmp_prlu_flag;
        return true;
    }

    bool CacheOperate::retrieve_victim_block_by_OPT(cache_info_st* cache, int set_idx, int& block_index)
    {
        if (nullptr == cache
            || set_idx >= cache->cfg.sets_num)                 // empty pointer or violate the index limit,false
        {
            return false;
        }
        int opt_max = INT_MAX;                                //default int max
        std::vector<int> optset;
        for (int i = 0; i < cache->cfg.assoc_num; i++) {
            optset.push_back(cache->sets[set_idx].p_blocks[i].memblock_addr);
            //optset.push_back(cache->sets[set_idx].p_blocks[i].tag);
            //optset.push_back(cache->sets[set_idx].p_blocks[i].hex_addr.op_addr);
        }

        //next_cache_write_block.hex_addr.op_times
 

       // std::cout << op_times1 << "\n";
       // std::cout << CacheOperate::alladdr.size() << "\n";//test alladdr 

        if (optset.size() > 1) {
            for (int i = op_times1 +1; i < CacheOperate::alladdr.size(); i++) //start point:current operation times 
            {
                int set_index = 0, tag = 0, mem_addr_by_block = 0;
                retrieve_block_info(CacheOperate::alladdr[i], cache->cfg, tag, set_index, mem_addr_by_block);
    
                // optset vector store memblock_addr of all assoc at set_idx
                    for (int j = 0; j < optset.size(); j++)
                    {
                       if (optset[j] == mem_addr_by_block) // if memblock_addr in optset vetcor (L1 or L2 certain index) match in trace 
                       // if (optset[j] == CacheOperate::alladdr[i])
                        {
                            optset.erase(optset.begin() + j);              // begin()=0,remove the (0+j)th element in optset vector
                            break;
                        }
                    }
                
            }
            if (optset.size() == 0) {
                for (int i = 0; i < cache->cfg.assoc_num; i++) {
                    optset.push_back(cache->sets[set_idx].p_blocks[i].memblock_addr);
                }
                for (int i = op_times1 + 1; i < CacheOperate::alladdr.size(); i++) //start point:current operation times 
                {
                    int set_index = 0, tag = 0, mem_addr_by_block = 0;
                    retrieve_block_info(CacheOperate::alladdr[i], cache->cfg, tag, set_index, mem_addr_by_block);

                    // optset vector store memblock_addr of all assoc at set_idx
                    for (int j = 0; j < optset.size()-1; j++)
                    {
                        if (optset[j] == mem_addr_by_block) // if memblock_addr in optset vetcor (L1 or L2 certain index) match in trace 
                        // if (optset[j] == CacheOperate::alladdr[i])
                        {
                            optset.erase(optset.begin() + j);              // begin()=0,remove the (0+j)th element in optset vector
                            break;
                        }
                    }

                }
            }
        }

        int victimAddr = optset[0]; // some of them that will show up in future are removed from set vector
                                            // blockAdds left in set vector are candidates of victim block, optset[0] is the farest one showing up in future
        for (int j = 0; j < cache->cfg.assoc_num; j++) {
            if (cache->sets[set_idx].p_blocks[j].memblock_addr == victimAddr){
            //if (cache->sets[set_idx].p_blocks[j].tag == victimAddr)
           // if (cache->sets[set_idx].p_blocks[j].hex_addr.op_addr == victimAddr) { // find the victim block position
                opt_max = cache->sets[set_idx].p_blocks[j].opt_times; 
                block_index = j;
                break;
            }
        }
        return true;
    }
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
    bool CacheOperate::retrieve_empty_block(cache_info_st* cache, int set_idx, int& block_index)
    {
        if (nullptr == cache)
        {
            return false;
        }
        for (int i = 0; i < cache->cfg.assoc_num; i++)
        {
            if (!cache->sets[set_idx].p_blocks[i].valid)  // if its valid=false, it's an empty block
            {
                block_index = i;
                return true;
            }
        }
        return false;
    }

    bool CacheOperate::op_write_back(cache_info_st* cache, int op_addr)
    {
        return false;
    }

    bool CacheOperate::update_prlu_flag(cache_info_st* cache, int set_index, int acess_block_index)
    {
        if (nullptr == cache)
        {
            return false;
        }
        int old_prlu_flag = cache->sets[set_index].prlu_flag;
        int prlu_flag_len = cache->cfg.assoc_num - 1;
        for (int i = prlu_flag_len; i > 0; i--)
        {
            //old_prlu_flag ^= (acess_block_index&(1<<(i-1)));
            old_prlu_flag &= ~(1 << (i - 1));
        }
        cache->sets[set_index].prlu_flag = old_prlu_flag;
        return true;
    }


    bool CacheOperate::op_mem_data(cache_info_st* cache, char op_type, int op_addr,int op_times)
    {
        op_times1 = op_times; // operation count
        cache_block_st op_block;
        if ('w' == op_type)
        {
            return op_write_data(cache, op_addr, op_block);
        }
        else if ('r' == op_type)
        {
            return op_read_data(cache, op_addr, op_block);
        }
        else
        {
            return false;
        }
    }

    bool CacheOperate::op_write_data(cache_info_st* cache, int op_addr, cache_block_st& op_block)
    {
        if (nullptr == cache)
        {
            return false;
        }
        int set_index = 0, tag = 0, mem_addr_by_block = 0;
        if (!retrieve_block_info(op_addr, cache->cfg, tag, set_index, mem_addr_by_block)) //retrieve data fails
        {
            return false;
        }
        bool write_hit = false;
        cache->sta.write_times++;                        //if retrieve block succeeds, write_tmes increments 1
        //cache->sets[set_index].current_max_rlu++;
        for (int i = 0; i < cache->cfg.assoc_num; i++)   /*1st find a match  block*/
        {
            if (cache->sets[set_index].p_blocks[i].valid
                && (cache->sets[set_index].p_blocks[i].tag == tag)) //if valid=true and match tag
            {
                cache->sets[set_index].p_blocks[i].dirty = true;    //only if write hit, dirty=true
                cache->sets[set_index].p_blocks[i].memblock_addr = mem_addr_by_block; //do write operation replacing current addr
                cache->sets[set_index].p_blocks[i].hex_addr.op_addr = op_addr;
                cache->sets[set_index].p_blocks[i].lru_times = cache->sets[set_index].current_max_rlu++;
                cache->sets[set_index].p_blocks[i].opt_times = cache->sets[set_index].current_max_opt++;
                write_hit = true;
                update_prlu_flag(cache, set_index, i);
                /*HIT*/         
                //break; // break just stops this loop
                //return true;// return means once hit, stop the function
                return 0;// return means once hit, stop the function
            }
        }

        // if miss in L1 above, then
        //go to next level cache (L2)
        if (!write_hit)  //find a valid (empty) block, if miss in L1(cache) and L2 (next_catche)
        {
            int free_block_index = 0;
            bool bop = retrieve_empty_block(cache, set_index, free_block_index);
            if (bop)

            {   //copy the current free block to op_block (struct variable)
                memcpy_s(&op_block, sizeof(cache_block_st), &(cache->sets[set_index].p_blocks[free_block_index]), sizeof(cache_block_st));

                if (nullptr != cache->next_catche) {
                    cache_block_st op_block_next_cache;
                    op_read_data(cache->next_catche, op_addr, op_block_next_cache);
                }
                
                cache->sets[set_index].p_blocks[free_block_index].valid = true;
                cache->sets[set_index].p_blocks[free_block_index].dirty = true;
                cache->sets[set_index].p_blocks[free_block_index].tag = tag;
                cache->sets[set_index].p_blocks[free_block_index].memblock_addr = mem_addr_by_block;
                cache->sets[set_index].p_blocks[free_block_index].hex_addr.op_addr = op_addr;
                cache->sta.write_missed_times++;
                cache->sets[set_index].p_blocks[free_block_index].lru_times = cache->sets[set_index].current_max_rlu++;
                cache->sets[set_index].p_blocks[free_block_index].opt_times = cache->sets[set_index].current_max_opt++;
                update_prlu_flag(cache, set_index, free_block_index);
			    //write_hit = true;/*HIT*/

            }
            else    //if no empty blocks in L1 or L2, then find victim blocks in L1 or L2
            {
                retrieve_victim_block(cache, cache->cfg.method_to_find_victim_block, set_index, free_block_index);
                if (cache->sets[set_index].p_blocks[free_block_index].dirty) //if dirty block, writeback to next level of mem
                {   
                    //copy the current victim block to op_block
					memcpy_s(&op_block, sizeof(cache_block_st), &(cache->sets[set_index].p_blocks[free_block_index]), sizeof(cache_block_st));
                    //todo write back
                    cache->sets[set_index].p_blocks[free_block_index].valid = false; //evict victim and invalidate it
                   // cache->sta.writeback_times++;

                    cache_block_st next_cache_write_block;
					if (nullptr != cache->next_catche)
					{
						bool op = op_write_data(cache->next_catche, cache->sets[set_index].p_blocks[free_block_index].hex_addr.op_addr, next_cache_write_block);
						
                        //L2 miss and evict a valid block, then for inclusive, also need to evict it in L1 if it exists 
                        if ((op==true)                               //L2 miss
							&& ( next_cache_write_block.valid	)    //L2 evict a valid block
							&& (1 == cache->cfg.inclusion_property)) // inclusive mode
						{
							int sub_set_index = 0, sub_tag = 0, sub_mem_addr_by_block = 0;
							retrieve_block_info(next_cache_write_block.hex_addr.op_addr,
								cache->cfg, sub_tag, sub_set_index, sub_mem_addr_by_block); //retrieve block info in L1 by the evicted op_addr from L2.op_block
							
								for (int i = 0; i < cache->cfg.assoc_num; i++)   /*1st find a match  block*/
								{
                                 // if ((cache->sets[sub_set_index].p_blocks[i].memblock_addr == sub_mem_addr_by_block )
									if ((cache->sets[sub_set_index].p_blocks[i].tag == sub_tag )
										&& cache->sets[sub_set_index].p_blocks[i].valid)
									{
										cache->sets[sub_set_index].p_blocks[i].valid = false;
										if (cache->sets[sub_set_index].p_blocks[i].dirty)
										{
											cache->sta.writeback_times1++;
                                            //cache->next_catche->sta.writeback_times--;
										}
										else
										{
											//cache->next_catche->sta.writeback_times++;
										}
										break;
									}
								}
							
						}
                        else {// if L2 hit, no need to evict block in L1. 
                            cache->sta.writeback_times++;
                        }
					}  
					else
                    {
                        //if no next level cache, just writeback++ for current level cache
						cache->sta.writeback_times++;
					}
                }
                else {// if victim block is not dirty,just valid,then only invalidate it
                    cache->sets[set_index].p_blocks[free_block_index].valid = false; //evict victim and invalidate it
                }
				
                if (nullptr != cache->next_catche) 
				{   cache_block_st op_block_next_cache;
                    op_read_data(cache->next_catche, op_addr, op_block_next_cache);
                }
                cache->sets[set_index].p_blocks[free_block_index].valid = true;
                cache->sets[set_index].p_blocks[free_block_index].dirty = true;
                cache->sets[set_index].p_blocks[free_block_index].tag = tag;
                cache->sets[set_index].p_blocks[free_block_index].memblock_addr = mem_addr_by_block;
                cache->sets[set_index].p_blocks[free_block_index].hex_addr.op_addr = op_addr;
                cache->sta.write_missed_times++;
                cache->sets[set_index].p_blocks[free_block_index].lru_times = cache->sets[set_index].current_max_rlu++;
                cache->sets[set_index].p_blocks[free_block_index].opt_times = cache->sets[set_index].current_max_opt++;
                update_prlu_flag(cache, set_index, free_block_index);
                //write_hit = true;/*HIT*/
            }
            
        }
        return true;
    }

    bool CacheOperate::op_read_data(cache_info_st* cache, int op_addr, cache_block_st& op_block)
    {
        if (nullptr == cache)
        {
            return false;
        }
        int set_index = 0, tag = 0, mem_addr_by_block = 0;
        if (!retrieve_block_info(op_addr, cache->cfg, tag, set_index, mem_addr_by_block))
        {
            return false;
        }
        bool read_hit = false;
        cache->sta.read_times++;
        //cache->sets[set_index].current_max_rlu++;
        for (int i = 0; i < cache->cfg.assoc_num; i++)   /*1st find a match'tag' block*/
        {
            if (cache->sets[set_index].p_blocks[i].valid
                && (cache->sets[set_index].p_blocks[i].tag == tag))
            {

                cache->sets[set_index].p_blocks[i].memblock_addr = mem_addr_by_block;
                cache->sets[set_index].p_blocks[i].hex_addr.op_addr = op_addr;
                cache->sets[set_index].p_blocks[i].lru_times = cache->sets[set_index].current_max_rlu++;
                cache->sets[set_index].p_blocks[i].opt_times = cache->sets[set_index].current_max_opt++;
                read_hit = true;
                update_prlu_flag(cache, set_index, i);
                op_block = cache->sets[set_index].p_blocks[i];
                //break;
                return 0;
            }
        }

        if (!read_hit)
        {
         
            int free_block_idx = 0;  ///*2nd find an empty block*/
            bool bop = retrieve_empty_block(cache, set_index, free_block_idx);
            if (bop)   //found a empty block 
            {
                //copy the current free block to op_block (struct variable)
                memcpy_s(&op_block, sizeof(cache_block_st), &(cache->sets[set_index].p_blocks[free_block_idx]), sizeof(cache_block_st));

                if (nullptr != cache->next_catche) {
                    cache_block_st op_block_next_cache;
                    op_read_data(cache->next_catche, op_addr, op_block_next_cache);
                }
                cache->sets[set_index].p_blocks[free_block_idx].valid = true;
                cache->sets[set_index].p_blocks[free_block_idx].dirty = false;
                cache->sets[set_index].p_blocks[free_block_idx].tag = tag;
                cache->sets[set_index].p_blocks[free_block_idx].memblock_addr = mem_addr_by_block;
                cache->sets[set_index].p_blocks[free_block_idx].hex_addr.op_addr = op_addr;
                cache->sets[set_index].p_blocks[free_block_idx].lru_times = cache->sets[set_index].current_max_rlu++;
                cache->sets[set_index].p_blocks[free_block_idx].opt_times = cache->sets[set_index].current_max_opt++;
                update_prlu_flag(cache, set_index, free_block_idx);
                cache->sta.read_missed_times++;
            }
            else //3rd have to find an victim block
            {
                retrieve_victim_block(cache, cache->cfg.method_to_find_victim_block, set_index, free_block_idx);
                if (cache->sets[set_index].p_blocks[free_block_idx].dirty)
                {   
                    //copy the current free block to op_block (struct variable)
                    memcpy_s(&op_block, sizeof(cache_block_st), &(cache->sets[set_index].p_blocks[free_block_idx]), sizeof(cache_block_st));

                    cache->sets[set_index].p_blocks[free_block_idx].dirty = false;//evict
                    //cache->sta.writeback_times++;

                    cache_block_st next_cache_write_block;
                    if (nullptr != cache->next_catche)
                    {
                        bool op=op_write_data(cache->next_catche, cache->sets[set_index].p_blocks[free_block_idx].hex_addr.op_addr, next_cache_write_block);
                        if ((op == true)
                            && (next_cache_write_block.valid)
                            && (1 == cache->cfg.inclusion_property)) {// L2 miss, evict a valid block,so L1 also need to evict it if it exists
                            int sub_set_index = 0, sub_tag = 0, sub_mem_addr_by_block = 0;
                            retrieve_block_info(next_cache_write_block.hex_addr.op_addr,
                                cache->cfg, sub_tag, sub_set_index, sub_mem_addr_by_block); //retrieve data fails
                            
                                for (int i = 0; i < cache->cfg.assoc_num; i++)   /*1st find a match  block*/
                                {
                                    if ((cache->sets[sub_set_index].p_blocks[i].tag == sub_tag)
                                        && cache->sets[sub_set_index].p_blocks[i].valid)
                                    {
                                        cache->sets[sub_set_index].p_blocks[i].valid = false;
                                        if (cache->sets[sub_set_index].p_blocks[i].dirty)
                                        {
                                            cache->sta.writeback_times1++;     // L1 writeback to main mem, not L2
                                            //cache->next_catche->sta.writeback_times--;// if L2 victim dirty=1, L2writeback++ has be done in branch L2writemiss of op_write_data() 
                                        }
                                        else
                                        {
                                            //cache->next_catche->sta.writeback_times++;
                                        }
                                        break;
                                    }
                                }
                            

                        }
                        else {// if op=others, L2 hit, L1 just writeback++
                            cache->sta.writeback_times++;
                        }
                    }
                    else // if (nullptr != cache->next_catche) isn't executed, just do writeback. Eg, when L2 r/w, L3 doesn't exist, no this if branch
                    {    //when L1 r/w, L2 exists, this if branch exists. valid victim B of L2 is evicted, copy L2.B.block to op_block, let L2.B.valid=false;
                           //for inclusive=1, L1 also evicts B: use op_block.hexadd.op_addr to find L1.B.valid; if L1.B.valid=true, then if L1.B.dirty=1, L1writeback1++,
                        cache->sta.writeback_times++;
                    }
                }
                else {
                    cache->sets[set_index].p_blocks[free_block_idx].dirty = false;
                }
                if (nullptr != cache->next_catche) {
                    cache_block_st op_block_next_cache;
                    op_read_data(cache->next_catche, op_addr, op_block_next_cache);
                }
                cache->sets[set_index].p_blocks[free_block_idx].valid = true;
                cache->sets[set_index].p_blocks[free_block_idx].dirty = false;
                cache->sets[set_index].p_blocks[free_block_idx].tag = tag;
                //cache->sets[set_index].p_blocks[free_block_idx].lru_times = 0;
                cache->sets[set_index].p_blocks[free_block_idx].memblock_addr = mem_addr_by_block;
                cache->sets[set_index].p_blocks[free_block_idx].hex_addr.op_addr = op_addr;
                cache->sets[set_index].p_blocks[free_block_idx].lru_times = cache->sets[set_index].current_max_rlu++;
                cache->sets[set_index].p_blocks[free_block_idx].opt_times = cache->sets[set_index].current_max_opt++;
                update_prlu_flag(cache, set_index, free_block_idx);
                cache->sta.read_missed_times++;
            }
        }
        return true;
    }
}