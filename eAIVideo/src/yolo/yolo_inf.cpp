

#include "yolo.hpp"


/*Variable for Performance Measurement*/
static struct timespec start_time;
static struct timespec inf_end_time;

static int8_t load_drpai_data(int8_t drpai_fd);
static int8_t load_data_to_mem(string data, int8_t drpai_fd, uint32_t from, uint32_t size);
static int8_t read_addrmap_txt(string addr_file);
static vector<string> load_label_file(string label_file_name);
int8_t get_result(int8_t drpai_fd, uint32_t output_addr, uint32_t output_size);

YoloInference::YoloInference( )
{
    this->width   = DRPAI_IN_WIDTH;
    this->height  = DRPAI_IN_HEIGHT;
    this->channel = DRPAI_IN_CHANNEL_BGR;
    this->size = width * height * channel;
    
    this->img.init( this->width, this->height, this->channel );

}

YoloInference::~YoloInference()
{
}

int8_t YoloInference::init()
{
    int8_t ret = 0;
    /**********************************************************************/
    /* Obtain udmabuf memory area starting address 						  */
    /**********************************************************************/
    uint64_t udmabuf_address = 0;
    int8_t fd = 0;
    char addr[1024];
    int32_t read_ret = 0;
    errno = 0;
    fd = open("/sys/class/u-dma-buf/udmabuf0/phys_addr", O_RDONLY);
    if (0 > fd)
    {
        fprintf(stderr, "[ERROR] Failed to open udmabuf0/phys_addr : errno=%d\n", errno);
        return -1;
    }
    read_ret = read(fd, addr, 1024);
    if (0 > read_ret)
    {
        fprintf(stderr, "[ERROR] Failed to read udmabuf0/phys_addr : errno=%d\n", errno);
        close(fd);
        return -1;
    }
    sscanf(addr, "%lx", &udmabuf_address);
    close(fd);
    /* Filter the bit higher than 32 bit */
    udmabuf_address &=0xFFFFFFFF;
    printf("Input Buffer Address: %lx\n", (uintptr_t)udmabuf_address);

    /**********************************************************************/
    /* Create and initialize DRP Input buffer                                            */
    /**********************************************************************/

    udmabuf_fd = open("/dev/udmabuf0", O_RDWR );
    if (udmabuf_fd < 0)
    {
        return -1;
    }
    inBuffer =(uint8_t*) mmap(NULL, this->size, PROT_READ|PROT_WRITE, MAP_SHARED,  udmabuf_fd, 0);

    if (inBuffer == MAP_FAILED)
    {
        return -1;
    }
    

    /* Write once to allocate physical memory to u-dma-buf virtual space.
    * Note: Do not use memset() for this.
    *       Because it does not work as expected. */
    {
        for (int i = 0 ; i < this->size; i++)
        {
            inBuffer[i] = 0;
        }
    }
    // Set Image Buffer AI input buffer
    this->img.img_buffer = inBuffer;
    
    /**********************************************************************/
	/* Inference preparation                                              */
	/**********************************************************************/
    printf("Inference preparation \n");
    /* Read DRP-AI Object files address and size */
    ret = read_addrmap_txt(drpai_address_file);
    if (0 != ret)
    {
        printf("[ERROR] Failed to read addressmap text file: %s\n", drpai_address_file.c_str());
        ret = -1;
        goto end_main;
    }
    
#if defined(YOLOV3) || defined(TINYYOLOV3)
    /*Load Label from label_list file*/
    label_file_map = load_label_file(label_list);
    if (label_file_map.empty())
    {
        printf("[ERROR] Failed to load label file: %s\n", label_list.c_str());
        ret = -1;
        goto end_main;
    } else {
    	printf("%s\n", label_file_map[0].c_str());
    }
#endif

    drpai_fd = open("/dev/drpai0", O_RDWR);
    if (0 > drpai_fd)
    {
        printf("[ERROR] Failed to open DRP-AI Driver: errno=%d\n", errno);
        ret = -1;
        goto end_main;
    }

    /* Load DRP-AI Data from Filesystem to Memory via DRP-AI Driver */
    ret = load_drpai_data(drpai_fd);
    if (0 > ret)
    {
        printf("[ERROR] Failed to load DRP-AI Object files.\n");
        ret = -1;
        goto end_close_drpai;
    }

    
    /* Set DRP-AI Driver Input (DRP-AI Object files address and size)*/
    proc[DRPAI_INDEX_INPUT].address       = udmabuf_address;
    proc[DRPAI_INDEX_INPUT].size          = drpai_address.data_in_size;
    proc[DRPAI_INDEX_DRP_CFG].address     = drpai_address.drp_config_addr;
    proc[DRPAI_INDEX_DRP_CFG].size        = drpai_address.drp_config_size;
    proc[DRPAI_INDEX_DRP_PARAM].address   = drpai_address.drp_param_addr;
    proc[DRPAI_INDEX_DRP_PARAM].size      = drpai_address.drp_param_size;
    proc[DRPAI_INDEX_AIMAC_DESC].address  = drpai_address.desc_aimac_addr;
    proc[DRPAI_INDEX_AIMAC_DESC].size     = drpai_address.desc_aimac_size;
    proc[DRPAI_INDEX_DRP_DESC].address    = drpai_address.desc_drp_addr;
    proc[DRPAI_INDEX_DRP_DESC].size       = drpai_address.desc_drp_size;
    proc[DRPAI_INDEX_WEIGHT].address      = drpai_address.weight_addr;
    proc[DRPAI_INDEX_WEIGHT].size         = drpai_address.weight_size;
    proc[DRPAI_INDEX_OUTPUT].address      = drpai_address.data_out_addr;
    proc[DRPAI_INDEX_OUTPUT].size         = drpai_address.data_out_size;
    
    /*DRP-AI Output Memory Preparation*/
    drpai_outdata.address = drpai_address.data_out_addr;
    drpai_outdata.size = drpai_address.data_out_size;

    goto end_main;
   
    /* Terminating process */
end_close_drpai:
    printf("eAI Close drpai file\n");
    errno = 0;
    ret = close(drpai_fd);
    if (0 != ret)
    {
        printf("[ERROR] Failed to close DRP-AI Driver: errno=%d\n", errno);
        ret = -1;
    }
    goto end_main;

end_main:
    return ret;
}

int8_t YoloInference::start()
{
    int8_t ret = 0;
    
    /**********************************************************************
    * START Inference
    **********************************************************************/
    printf("[START] DRP-AI\n");
    timespec_get(&start_time, TIME_UTC);
    ret = ioctl(drpai_fd, DRPAI_START, &proc[0]);
    if (0 != ret)
    {
        printf("[ERROR] Failed to run DRPAI_START: errno=%d\n", errno);
        ret = -1;
        
    }   
    return ret;    
}

int8_t YoloInference::wait()
{
    int8_t ret = 0;
    int8_t ret_drpai = 0;
    fd_set rfds;
    struct timeval tv;
    drpai_status_t drpai_status;
    
    /**********************************************************************
    * Wait until the DRP-AI finish (Thread will sleep)
    **********************************************************************/
    FD_ZERO(&rfds);
    FD_SET(drpai_fd, &rfds);
    tv.tv_sec = DRPAI_TIMEOUT;
    tv.tv_usec = 0;
    
    ret_drpai = select(drpai_fd+1, &rfds, NULL, NULL, &tv);

    /*Gets AI Inference End Time*/
    timespec_get(&inf_end_time, TIME_UTC);

    if (0 == ret_drpai)
    {
        printf("[ERROR] DRP-AI select() Timeout : errno=%d\n", errno);
        ret = -1;
        goto end_wait;
    }
    else if (-1 == ret_drpai)
    {
        printf("[ERROR] DRP-AI select() Error : errno=%d\n", errno);
        ret_drpai = ioctl(drpai_fd, DRPAI_GET_STATUS, &drpai_status);
        if (-1 == ret_drpai)
        {
            printf("[ERROR] Failed to run DRPAI_GET_STATUS : errno=%d\n", errno);
        }
        ret = -1;
        goto end_wait;
    }
    else
    {
        /*Do nothing*/
    }
    
    if (FD_ISSET(drpai_fd, &rfds))
    {
        errno = 0;
        ret_drpai = ioctl(drpai_fd, DRPAI_GET_STATUS, &drpai_status);
        if (-1 == ret_drpai)
        {
            printf("[ERROR] Failed to run DRPAI_GET_STATUS : errno=%d\n", errno);
            ret = -1;
            goto end_wait;
        }
        //printf("[END] DRP-AI\n");
    }
end_wait:
    return ret;
}

uint8_t* YoloInference::getInputBuf()
{
    return inBuffer;
}

uint32_t YoloInference::getInImageSize( void )
{
    return this->size;
}
void YoloInference::printPerformance ( void ) {

	std::stringstream stream;
	std::string str = "";
	static uint32_t ai_inf_prev = 0;
	static float ai_time = 0;

	ai_time = (float)((timedifference_msec(start_time, inf_end_time)));

	if (ai_inf_prev != (uint32_t) ai_time)
	{
		ai_inf_prev = (uint32_t) ai_time;
		printf("DRP-AI Time: %d msec\n",  ai_inf_prev);
	}
}
/*****************************************
* Function Name : read_addrmap_txt
* Description   : Loads address and size of DRP-AI Object files into struct addr.
* Arguments     : addr_file = filename of addressmap file (from DRP-AI Object files)
* Return value  : 0 if succeeded
*                 not 0 otherwise
******************************************/

int8_t YoloInference::read_addrmap_txt(string addr_file)
{
    string str;
    uint32_t l_addr;
    uint32_t l_size;
    string element, a, s;

    ifstream ifs(addr_file);
    if (ifs.fail())
    {
        printf("[ERROR] Failed to open address map list : %s\n", addr_file.c_str());
        return -1;
    }

    while (getline(ifs, str))
    {
        istringstream iss(str);
        iss >> element >> a >> s;
        l_addr = strtol(a.c_str(), NULL, 16);
        l_size = strtol(s.c_str(), NULL, 16);

        if ("drp_config" == element)
        {
            drpai_address.drp_config_addr = l_addr;
            drpai_address.drp_config_size = l_size;
        }
        else if ("desc_aimac" == element)
        {
            drpai_address.desc_aimac_addr = l_addr;
            drpai_address.desc_aimac_size = l_size;
        }
        else if ("desc_drp" == element)
        {
            drpai_address.desc_drp_addr = l_addr;
            drpai_address.desc_drp_size = l_size;
        }
        else if ("drp_param" == element)
        {
            drpai_address.drp_param_addr = l_addr;
            drpai_address.drp_param_size = l_size;
        }
        else if ("weight" == element)
        {
            drpai_address.weight_addr = l_addr;
            drpai_address.weight_size = l_size;
        }
        else if ("data_in" == element)
        {
            drpai_address.data_in_addr = l_addr;
            drpai_address.data_in_size = l_size;
        }
        else if ("data" == element)
        {
            drpai_address.data_addr = l_addr;
            drpai_address.data_size = l_size;
        }
        else if ("data_out" == element)
        {
            drpai_address.data_out_addr = l_addr;
            drpai_address.data_out_size = l_size;
        }
        else if ("work" == element)
        {
            drpai_address.work_addr = l_addr;
            drpai_address.work_size = l_size;
        }
        else
        {
            /*Ignore other space*/
        }
    }

    return 0;
}

/*****************************************
* Function Name : load_data_to_mem
* Description   : Loads a file to memory via DRP-AI Driver
* Arguments     : data = filename to be written to memory
*                 drpai_fd = file descriptor of DRP-AI Driver
*                 from = memory start address where the data is written
*                 size = data size to be written
* Return value  : 0 if succeeded
*                 not 0 otherwise
******************************************/

static int8_t load_data_to_mem(string data, int8_t drpai_fd, uint32_t from, uint32_t size)
{
    int8_t ret_load_data = 0;
    int8_t obj_fd;
    uint8_t drpai_buf[BUF_SIZE];
    drpai_data_t drpai_data;
    size_t ret = 0;
    int32_t i = 0;

    printf("Loading : %s\n", data.c_str());
    errno = 0;
    obj_fd = open(data.c_str(), O_RDONLY);
    if (0 > obj_fd)
    {
        printf("[ERROR] Failed to open: %s errno=%d\n", data.c_str(), errno);
        ret_load_data = -1;
        goto end;
    }

    drpai_data.address = from;
    drpai_data.size = size;

    errno = 0;
    ret = ioctl(drpai_fd, DRPAI_ASSIGN, &drpai_data);
    if ( -1 == ret )
    {
        printf("[ERROR] Failed to run DRPAI_ASSIGN: errno=%d\n", errno);
        ret_load_data = -1;
        goto end;
    }

    for (i = 0; i < (drpai_data.size / BUF_SIZE); i++)
    {
        errno = 0;
        ret = read(obj_fd, drpai_buf, BUF_SIZE);
        if ( 0 > ret )
        {
            printf("[ERROR] Failed to read: %s errno=%d\n", data.c_str(), errno);
            ret_load_data = -1;
            goto end;
        }
        ret = write(drpai_fd, drpai_buf, BUF_SIZE);
        if ( -1 == ret )
        {
            printf("[ERROR] Failed to write via DRP-AI Driver: errno=%d\n", errno);
            ret_load_data = -1;
            goto end;
        }
    }
    if ( 0 != (drpai_data.size % BUF_SIZE))
    {
        errno = 0;
        ret = read(obj_fd, drpai_buf, (drpai_data.size % BUF_SIZE));
        if ( 0 > ret )
        {
            printf("[ERROR] Failed to read: %s errno=%d\n", data.c_str(), errno);
            ret_load_data = -1;
            goto end;
        }
        ret = write(drpai_fd, drpai_buf, (drpai_data.size % BUF_SIZE));
        if ( -1 == ret )
        {
            printf("[ERROR] Failed to write via DRP-AI Driver: errno=%d\n", errno);
            ret_load_data = -1;
            goto end;
        }
    }
    goto end;

end:
    if (0 < obj_fd)
    {
        close(obj_fd);
    }
    return ret_load_data;
}

/*****************************************
* Function Name : load_drpai_data
* Description   : Loads DRP-AI Object files to memory via DRP-AI Driver.
* Arguments     : drpai_fd = file descriptor of DRP-AI Driver
* Return value  : 0 if succeeded
*               : not 0 otherwise
******************************************/
int8_t YoloInference::load_drpai_data(int8_t drpai_fd)
{
    uint32_t addr = 0;
    uint32_t size = 0;
    int32_t i = 0;
    size_t ret = 0;
    for ( i = 0; i < 5; i++ )
    {
        switch (i)
        {
            case (INDEX_W):
                addr = drpai_address.weight_addr;
                size = drpai_address.weight_size;
                break;
            case (INDEX_C):
                addr = drpai_address.drp_config_addr;
                size = drpai_address.drp_config_size;
                break;
            case (INDEX_P):
                addr = drpai_address.drp_param_addr;
                size = drpai_address.drp_param_size;
                break;
            case (INDEX_A):
                addr = drpai_address.desc_aimac_addr;
                size = drpai_address.desc_aimac_size;
                break;
            case (INDEX_D):
                addr = drpai_address.desc_drp_addr;
                size = drpai_address.desc_drp_size;
                break;
            default:
                break;
        }

        ret = load_data_to_mem(drpai_file_path[i], drpai_fd, addr, size);
        if (0 != ret)
        {
            printf("[ERROR] Failed to load data from memory: %s\n",drpai_file_path[i].c_str());
            return -1;
        }
    }
    return 0;
}

/*****************************************
* Function Name     : load_label_file
* Description       : Load label list text file and return the label list that contains the label.
* Arguments         : label_file_name = filename of label list. must be in txt format
* Return value      : vector<string> list = list contains labels
*                     empty if error occured
******************************************/
static vector<string> load_label_file(string label_file_name)
{
    vector<string> list = {};
    vector<string> empty = {};
    ifstream infile(label_file_name);

    if (!infile.is_open())
    {
        return list;
    }

    string line = "";
    while (getline(infile,line))
    {
        list.push_back(line);
        if (infile.fail())
        {
            return empty;
        }
    }

    return list;
}

/*****************************************
* Function Name : timedifference_msec
* Description   : compute the time differences in ms between two moments
* Arguments     : t0 = start time
*                 t1 = stop time
* Return value  : the time difference in ms
******************************************/
double YoloInference::timedifference_msec(struct timespec t0, struct timespec t1)
{
    return (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1000000.0;
}
