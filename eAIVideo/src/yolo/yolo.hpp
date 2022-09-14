
/*DRPAI Driver Header*/
#include <linux/drpai.h>
/*Definition of Macros & other variables*/
#include "define.h"
#include "image.h"

using namespace std;

#define EAI_ERROR   (-1)

/*****************************************
* detection : Detected result
******************************************/

class YoloInference
{
    public:
        YoloInference();
        ~YoloInference();
        
        // Inference Methods
        int8_t init( void );
        int8_t start ( void );
        int8_t wait ( void );
        uint8_t* getInputBuf( void );
        float* getOutputBuf( void );
        uint32_t getInImageSize( void );
        
        void read_imagefile( std::string filename );
        void write_imagefile( std::string filename );

        // Post Processing Methods
       	int8_t get_result( void );
        void print_result_yolo( void );
        void print_numResutls( void );
        void render(void);

        void yolo_postproc(float* floatarr);
        
        // Performance
        void printPerformance ( void );
        double timedifference_msec(struct timespec t0, struct timespec t1);

    private:
        // Inference Input Image
        uint16_t height;
        uint16_t width;
        uint16_t channel;
        uint32_t size;

        std::vector<std::string> label_file_map = YOLO_LABEL_MAP;
        vector<detection> det;

        // DRP-AI Parameters
        drpai_data_t proc[DRPAI_INDEX_NUM];
        st_addr_t drpai_address;
        
        // DRP-AI Output Params
        drpai_data_t drpai_outdata;

        // DRP-AI Input Buffer
        uint8_t *inBuffer;
        float drpai_output_buf[num_inf_out];


        // CDMA and DRP-AI File descripturs
        int32_t udmabuf_fd;
        int32_t drpai_fd;
        
        Image img;

		int8_t load_drpai_data(int8_t drpai_fd);
		int8_t read_addrmap_txt(string addr_file);
       


};
