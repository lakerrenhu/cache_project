#include "cache_op.h"
#include <vector>
/*
<BLOCKSIZE> <L1_SIZE> <L1_ASSOC> <L2_SIZE> <L2_ASSOC>
<REPLACEMENT_POLICY> <INCLUSION_PROPERTY> <trace_file>
*/

const int NUM_ADDR_LEN = 32;
//std::vector<int> alladdr;

void set_cache_replacement_policy(cache_sim::cache_config_st&cfg, int replace_policy)
{
    switch (replace_policy)
    {
    case 0:
        cfg.method_to_find_victim_block = cache_sim::METHOD_LRU;
        break;
    case 1:
        cfg.method_to_find_victim_block = cache_sim::METHOD_PLRU;
        break;
    case 2:
        cfg.method_to_find_victim_block = cache_sim::METHOD_OPTIMAL;
        break;
    default:
        cfg.method_to_find_victim_block = cache_sim::METHOD_LRU;
        break;
    }
}

std::string translate_cache_replacement_policy(int replace_policy)
{
    switch (replace_policy)
    {
    case 0:
        return "LRU";
    case 1:
        return "Pseudo-LRU";
    case 2:
        return "OPTIMAL";
    default:
        return "LRU";
    }
}

std::string translate_cache_inclusion_property(int pro)
{
    switch (pro)
    {
    case 1:
        return "inclusive";
    case 0:
        return "non-inclusive";
    default:
        return "non-inclusive";
    }
}

/*
create a cache from params
*/
bool construct_caches(cache_sim::cache_info_st& cache,
    int BLOCKSIZE,
    int L1_SIZE,
    int L1_ASSOC,
    int L2_SIZE,
    int L2_ASSOC,
    int REPLACEMENT_POLICY,
    int INCLUSION_PROPERTY)
{
    cache.cfg.cache_size = L1_SIZE;
    cache.cfg.assoc_num = L1_ASSOC;
    cache.cfg.sets_num = (int)(cache.cfg.cache_size / (cache.cfg.assoc_num * BLOCKSIZE));
    cache.sets = new cache_sim::cache_set_st[cache.cfg.sets_num]; //
    for (int i = 0; i < cache.cfg.sets_num; i++)
    {
        cache.sets[i].assoc = L1_ASSOC;  //
		cache.sets[i].prlu_flag = (1 << (cache.cfg.assoc_num-1))-1 ;
        cache.sets[i].p_blocks = new cache_sim::cache_block_st[cache.sets[i].assoc];
        memset(cache.sets[i].p_blocks, 0, sizeof(cache_sim::cache_block_st)*cache.sets[i].assoc);
    }

    cache.cfg.cache_level = cache_sim::CACHE_L1;
    cache.cfg.len_index_bits = (int)(log(cache.cfg.sets_num) / log(2));
    cache.cfg.len_blockoffset_bits = (int)(log(BLOCKSIZE) / log(2));
    cache.cfg.len_tag_bits = NUM_ADDR_LEN - (cache.cfg.len_index_bits + cache.cfg.len_blockoffset_bits);
    set_cache_replacement_policy(cache.cfg, REPLACEMENT_POLICY);
    cache.cfg.inclusion_property = INCLUSION_PROPERTY;

    if (L2_SIZE > 0
        && L2_ASSOC > 0)
    {
        cache_sim::cache_info_st* next_level_cache = new (std::nothrow)cache_sim::cache_info_st();

        next_level_cache->cfg.cache_size = L2_SIZE;
        next_level_cache->cfg.assoc_num = L2_ASSOC;
        next_level_cache->cfg.sets_num = (int)(next_level_cache->cfg.cache_size / (next_level_cache->cfg.assoc_num * BLOCKSIZE));
        next_level_cache->cfg.len_index_bits = (int)(log(next_level_cache->cfg.sets_num) / log(2));
        next_level_cache->cfg.len_blockoffset_bits = (int)(log(BLOCKSIZE) / log(2));
        next_level_cache->cfg.len_tag_bits = NUM_ADDR_LEN - (next_level_cache->cfg.len_index_bits + next_level_cache->cfg.len_blockoffset_bits);
        set_cache_replacement_policy(next_level_cache->cfg, REPLACEMENT_POLICY);
        next_level_cache->cfg.cache_level = cache_sim::CACHE_L2;
        next_level_cache->next_catche = nullptr;

        next_level_cache->sets = new cache_sim::cache_set_st[next_level_cache->cfg.sets_num]; //
        for (int i = 0; i < next_level_cache->cfg.sets_num; i++)
        {
            next_level_cache->sets[i].assoc = L2_ASSOC;  //
			cache.sets[i].prlu_flag = (1 << ((next_level_cache->cfg.assoc_num)-1)) - 1;
            next_level_cache->sets[i].p_blocks = new cache_sim::cache_block_st[next_level_cache->sets[i].assoc];
            memset(next_level_cache->sets[i].p_blocks, 0, sizeof(cache_sim::cache_block_st)*next_level_cache->sets[i].assoc);
        }

        cache.next_catche = next_level_cache;
    }

    return true;
}

bool fill_cacheinfo_byparams(int argc, char *argv[], 
    cache_sim::cache_info_st& cache,
    std::string& trace_file)
{
    int param_idx = 1;
    const int max_param_count = 8 + 1;
    if (argc != max_param_count)
    {
        printf("Please input as <BLOCKSIZE> <L1_SIZE> <L1_ASSOC> <L2_SIZE> <L2_ASSOC>\
            <REPLACEMENT_POLICY> <INCLUSION_PROPERTY> <trace_file> ");
        return false;
    }
    int BLOCKSIZE = atoi(argv[param_idx++]);
    if (BLOCKSIZE <= 0)
    {
        printf("[ERR] BLOCKSIZE should be > 0, but the input is %d ", BLOCKSIZE);
        return false;
    }
    cache.cfg.block_size = BLOCKSIZE;
    int L1_SIZE = atoi(argv[param_idx++]);
    if (L1_SIZE <= 0)
    {
        printf("[ERR] BLOCKSIZE should be > 0, but the input is %d ", L1_SIZE);
        return false;
    }

    int L1_ASSOC = atoi(argv[param_idx++]);
    if (L1_ASSOC <= 0)
    {
        printf("[ERR] L1_ASSOC should be > 0, but the input is %d ", L1_ASSOC);
        return false;
    }

    int L2_SIZE = atoi(argv[param_idx++]);
    int L2_ASSOC = atoi(argv[param_idx++]);
    int REPLACEMENT_POLICY  = atoi(argv[param_idx++]);
    if (REPLACEMENT_POLICY < 0
        || REPLACEMENT_POLICY > 2)
    {
        printf("[ERR] REPLACEMENT_POLICY should be [0,2], but the input is %d ", REPLACEMENT_POLICY);
        return false;
    }
    int INCLUSION_PROPERTY = atoi(argv[param_idx++]);
    if (INCLUSION_PROPERTY < 0
        || INCLUSION_PROPERTY >= 2)
    {
        printf("[ERR] REPLACEMENT_POLICY should be [0,1], but the input is %d ", INCLUSION_PROPERTY);
        return false;
    }

    trace_file = argv[param_idx++];

    return construct_caches(cache, BLOCKSIZE,
        L1_SIZE, L1_ASSOC, L2_SIZE, L2_ASSOC, REPLACEMENT_POLICY, INCLUSION_PROPERTY);
}

void print_cache_info(cache_sim::cache_info_st& cache,const std::string& file)
{
    printf("===== Simulator configuration =====\r\n");
    printf("BLOCKSIZE:           %d\r\n", cache.cfg.block_size);
    printf("L1_SIZE: %d\r\n", cache.cfg.cache_size);
    printf("L1_ASSOC: %d\r\n", cache.cfg.assoc_num);
    printf("L2_SIZE: %d\r\n", cache.next_catche == nullptr ? 0: cache.next_catche->cfg.cache_size);
    printf("L2_ASSOC: %d\r\n", cache.next_catche == nullptr ? 0 : cache.next_catche->cfg.assoc_num);
    printf("REPLACEMENT POLICY: %s\r\n", translate_cache_replacement_policy(cache.cfg.method_to_find_victim_block).c_str());
    printf("INCLUSION PROPERTY: %s\r\n", translate_cache_inclusion_property(cache.cfg.inclusion_property).c_str());
    printf("trace_file: %s\r\n", file.c_str());
}

/*
===== Simulation results (raw) =====
a. number of L1 reads:        63640
b. number of L1 read misses:  46080
c. number of L1 writes:       36360
d. number of L1 write misses: 30360
e. L1 miss rate:              0.764400
f. number of L1 writebacks:   32464
g. number of L2 reads:        0
h. number of L2 read misses:  0
i. number of L2 writes:       0
j. number of L2 write misses: 0
k. L2 miss rate:              0
l. number of L2 writebacks:   0
m. total memory traffic:      108904

*/
void print_cache_result(cache_sim::cache_info_st& cache)
{
    printf("===== Simulation results (raw) =====\r\n");
    printf("a. number of L1 reads:           %d\r\n", cache.sta.read_times);
    printf("b. number of L1 read misses: %d\r\n", cache.sta.read_missed_times);
    printf("c. number of L1 writes: %d\r\n", cache.sta.write_times);
    printf("d. number of L1 write misses: %d\r\n", cache.sta.write_missed_times);
    printf("e. L1 miss rate: %f\r\n", (cache.sta.read_times + cache.sta.write_times) == 0? 0.0:
        (cache.sta.read_missed_times+ cache.sta.write_missed_times)/(1.0*cache.sta.read_times + cache.sta.write_times));
    printf("f. number of L1 writebacks: %d\r\n", cache.sta.writeback_times);
    
    printf("g. number of L2 reads:           %d\r\n", (nullptr == cache.next_catche) ? 0 : cache.next_catche->sta.read_times);
    //printf("g. number of L2 reads:           %d\r\n", (nullptr == cache.next_catche) ?0: (cache.sta.read_missed_times + cache.sta.write_missed_times));
    printf("h. number of L2 read misses: %d\r\n", (nullptr == cache.next_catche) ? 0 : cache.next_catche->sta.read_missed_times);
    printf("i. number of L2 writes: %d\r\n", (nullptr == cache.next_catche) ? 0 : cache.next_catche->sta.writeback_times);
    //printf("i. number of L2 writes: %d\r\n", (nullptr == cache.next_catche) ? 0 : cache.sta.writeback_times);
    printf("j. number of L2 write misses: %d\r\n", (nullptr == cache.next_catche) ? 0 : cache.next_catche->sta.write_missed_times);
    if (nullptr == cache.next_catche)
    {
        printf("k. L2 miss rate: 0.0\r\n");
    }
    else
    {
        printf("k. L2 miss rate: %f\r\n", (1.0*cache.sta.read_missed_times + cache.sta.write_missed_times) == 0 ? 0.0 :
            (cache.next_catche->sta.read_missed_times / (1.0*cache.sta.read_missed_times + cache.sta.write_missed_times)));
       //(cache.next_catche->sta.read_missed_times + cache.next_catche->sta.write_missed_times) / (1.0*cache.next_catche->sta.read_times + cache.next_catche->sta.write_times));
    }
    printf("l. number of L2 writebacks: %d\r\n", cache.next_catche == nullptr ? 0 : cache.next_catche->sta.writeback_times);
    if (nullptr == cache.next_catche)
    {   //no L2, total mem=L1 readmiss+L1 writemiss+L1 writeback
        printf("m. total memory traffic: %d\r\n", (cache.sta.read_missed_times + cache.sta.write_missed_times + cache.sta.writeback_times));
    }
    else
    {    //non-inclusive， L2 exists, total mem=L2 readmiss+L2 writemiss+L2 writeback
        if (cache.cfg.inclusion_property==0) {
            printf("m. total memory traffic: %d\r\n", (cache.next_catche->sta.read_missed_times + cache.next_catche->sta.write_missed_times + cache.next_catche->sta.writeback_times));
        }
        else {// inclusive,L2 exists, total mem=L2 readmiss+L2 writemiss+L2 writeback+L1 writeback1
            printf("m. total memory traffic: %d\r\n", 
                (cache.next_catche->sta.read_missed_times + cache.next_catche->sta.write_missed_times + cache.next_catche->sta.writeback_times+ cache.sta.writeback_times1));
        }
    }
    //printf("m. total memory traffic: %d\r\n", cache.next_catche == nullptr ? 0 : cache.next_catche->cfg.assoc_num);
}
//
// (cache.next_catche->sta.read_missed_times+cache.next_catche->sta.write_missed_times+cache.next_catche->sta.writeback_times)
void print_cache_detail(cache_sim::cache_info_st& cache)
{
    printf("===== L1 contents =====\r\n");
    for (int i = 0; i < cache.cfg.sets_num; i++)
    {
        printf("Set\t%d:    ", i);
        for (int j = 0; j < cache.cfg.assoc_num; j++)
        {
            if (cache.sets[i].p_blocks[j].valid)
            {
                printf("%8x %c", cache.sets[i].p_blocks[j].tag, cache.sets[i].p_blocks[j].dirty ? 'D' : ' ');
            }
            else
            {
                printf("--------");
            }
            printf("\t");
        }
        printf("\n");
    }

    if (nullptr != cache.next_catche)
    {
        printf("===== L2 contents =====\r\n");
        for (int i = 0; i < cache.next_catche->cfg.sets_num; i++)
        {
            printf("Set\t%d:    ", i);
            for (int j = 0; j < cache.next_catche->cfg.assoc_num; j++)
            {
                if (cache.next_catche->sets[i].p_blocks[j].valid)
                {
                    printf("%8x %c", cache.next_catche->sets[i].p_blocks[j].tag, cache.next_catche->sets[i].p_blocks[j].dirty ? 'D' : ' ');
                }
                else
                {
                    printf("--------");
                }
                printf("\t");
            }
            printf("\n");
        }
    }
}

/*************************************************
Function:        loadOPAddrs
Description:    load op adress
Input:             const std::string& trace_file file full path
                      std::vector<int>& all all address
                       std::vector<int>& r read address
                        std::vector<int>& w write address
Output:         none
Return:        none
Others:
*************************************************/
void loadOPAddrs(const std::string& trace_file, std::vector<int>& all, std::vector<int>& r, std::vector<int>& w)
{
    FILE* _fille = fopen(trace_file.c_str(), "r");
    if (_fille == nullptr) {
        printf("Can not open the trace file %s!\n", trace_file.c_str());
        return;
    }
    const int READ_BUF_LEN = 32;
    char read_buf[READ_BUF_LEN] = { 0 };
    int  op_failed = 0, op_addr = 0;
    char op_type = 0;
    while (fgets(read_buf, READ_BUF_LEN, _fille))
    {
        if (2 == sscanf(read_buf, "%c %x\n", &op_type, &op_addr))
        {
            all.push_back(op_addr);

            if ('r' == op_type)
            {
                r.push_back(op_addr);
            }
            else
            {
                w.push_back(op_addr);
            }
        }
    }
    fclose(_fille);
}


/*
<BLOCKSIZE> <L1_SIZE> <L1_ASSOC> <L2_SIZE> <L2_ASSOC>
<REPLACEMENT_POLICY> <INCLUSION_PROPERTY> <trace_file>
*/
int main(int argc, char *argv[])
{
    //check input params 1st
    std::string trace_file;
    cache_sim::cache_info_st cache;
    if (!fill_cacheinfo_byparams(argc, argv, cache, trace_file))
    {
        printf("Invalid Input ,please input as following format ,\r\
            <BLOCKSIZE> <L1_SIZE> <L1_ASSOC> <L2_SIZE> <L2_ASSOC>\
            <REPLACEMENT_POLICY> <INCLUSION_PROPERTY> <trace_file>");
        return -1;
    }
    print_cache_info(cache, trace_file);

    //load address
    std::vector<int> all;
    std::vector<int> r;
    std::vector<int> w;
    loadOPAddrs(trace_file, all, r, w);

    cache_sim::CacheOperate::alladdr = all;//store all address in trace
    
    //std::cout<<all.size()<<"\n";
    
    FILE* _fille = fopen(trace_file.c_str(), "r");
    if (_fille == nullptr) {
        printf("Can not open the trace file %s!\n", trace_file.c_str());
        return -1;
    }

    const int READ_BUF_LEN = 32;
    char read_buf[READ_BUF_LEN] = { 0 };
    int op_failed =0, op_addr = 0,op_times=0;
    int op_counter = 0;
    //int cache_sim::op_times ;
    char op_type = 0;
    
    while (fgets(read_buf, READ_BUF_LEN, _fille))
    {
        sscanf(read_buf, "%c %x\n", &op_type, &op_addr);
        //op_counter += 1;
        op_times += 1;
        //std::cout << op_counter << "\n";
        //cache_sim::op_times1 = op_counter;  //current operation count
        //std::cout << cache_sim::op_times1 << "\n";
        if (!cache_sim::CacheOperate::op_mem_data(&cache, op_type, op_addr, op_times))
        //if (!cache_sim::CacheOperate::op_mem_data(&cache, op_type, op_addr))
        {
            op_failed += 1;
        }
    }
    fclose(_fille);
    print_cache_detail(cache);
    print_cache_result(cache);
    return 0;
}