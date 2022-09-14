#include "yolo.hpp"

double sigmoid(double x);
void softmax(float val[NUM_CLASS]);
int32_t yolo_offset(uint8_t n, int32_t b, int32_t y, int32_t x);
int32_t yolo_index(uint8_t n, int32_t offs, int32_t channel);

/*****************************************
* Function Name : R_Post_Proc
* Description   : Process CPU post-processing for Tiny YOLOv2
* Arguments     : floatarr = drpai output address
* Return value  : -
******************************************/
void YoloInference::yolo_postproc(float* floatarr)
{
	 /* Following variables are required for correct_yolo/region_boxes in Darknet implementation*/
	/* Note: This implementation refers to the "darknet detector test" */
	float new_w, new_h;
	float correct_w = 1.;
	float correct_h = 1.;
	if ((float) (MODEL_IN_W / correct_w) < (float) (MODEL_IN_H/correct_h) )
	{
		new_w = (float) MODEL_IN_W;
		new_h = correct_h * MODEL_IN_W / correct_w;
	}
	else
	{
		new_w = correct_w * MODEL_IN_H / correct_h;
		new_h = MODEL_IN_H;
	}

	int32_t n = 0;
	int32_t b = 0;
	int32_t y = 0;
	int32_t x = 0;
	int32_t offs = 0;
	int32_t i = 0;
	float tx = 0;
	float ty = 0;
	float tw = 0;
	float th = 0;
	float tc = 0;
	float center_x = 0;
	float center_y = 0;
	float box_w = 0;
	float box_h = 0;
	float objectness = 0;
	uint8_t num_grid = 0;
	uint8_t anchor_offset = 0;
	float classes[NUM_CLASS];
	float max_pred = 0;
	int32_t pred_class = -1;
	float probability = 0;
	detection d;
	/* Clear the detected result list */
	det.clear();

	for (n = 0; n<NUM_INF_OUT_LAYER; n++)
	{
		num_grid = num_grids[n];
		anchor_offset = 2 * NUM_BB * (NUM_INF_OUT_LAYER - (n + 1));

		for (b = 0;b<NUM_BB;b++)
		{
			for (y = 0;y<num_grid;y++)
			{
				for (x = 0;x<num_grid;x++)
				{
					offs = yolo_offset(n, b, y, x);
					tx = floatarr[offs];
					ty = floatarr[yolo_index(n, offs, 1)];
					tw = floatarr[yolo_index(n, offs, 2)];
					th = floatarr[yolo_index(n, offs, 3)];
					tc = floatarr[yolo_index(n, offs, 4)];

					/* Compute the bounding box */
					/*get_yolo_box/get_region_box in paper implementation*/
					center_x = ((float) x + sigmoid(tx)) / (float) num_grid;
					center_y = ((float) y + sigmoid(ty)) / (float) num_grid;
#if defined(YOLOV3) || defined(TINYYOLOV3)
					box_w = (float) exp(tw) * anchors[anchor_offset+2*b+0] / (float) MODEL_IN_W;
					box_h = (float) exp(th) * anchors[anchor_offset+2*b+1] / (float) MODEL_IN_W;
#elif defined(YOLOV2) || defined(TINYYOLOV2)
					box_w = (float) exp(tw) * anchors[anchor_offset+2*b+0] / (float) num_grid;
					box_h = (float) exp(th) * anchors[anchor_offset+2*b+1] / (float) num_grid;
#endif
					/* Adjustment for VGA size */
					/* correct_yolo/region_boxes */
					center_x = (center_x - (MODEL_IN_W - new_w) / 2. / MODEL_IN_W) / ((float) new_w / MODEL_IN_W);
					center_y = (center_y - (MODEL_IN_H - new_h) / 2. / MODEL_IN_H) / ((float) new_h / MODEL_IN_H);
					box_w *= (float) (MODEL_IN_W / new_w);
					box_h *= (float) (MODEL_IN_H / new_h);

					center_x = round(center_x * DRPAI_IN_WIDTH);
					center_y = round(center_y * DRPAI_IN_HEIGHT);
					box_w = round(box_w * DRPAI_IN_WIDTH);
					box_h = round(box_h * DRPAI_IN_HEIGHT);

					objectness = sigmoid(tc);

					Box bb = {center_x, center_y, box_w, box_h};
					/* Get the class prediction */
					for (i = 0;i < NUM_CLASS;i++)
					{
#if defined(YOLOV3) || defined(TINYYOLOV3)
						classes[i] = sigmoid(floatarr[yolo_index(n, offs, 5+i)]);
#elif defined(YOLOV2) || defined(TINYYOLOV2)
						classes[i] = floatarr[yolo_index(n, offs, 5+i)];
#endif
					}

#if defined(YOLOV2) || defined(TINYYOLOV2)
					softmax(classes);
#endif
					max_pred = 0;
					pred_class = -1;
					for (i = 0; i < NUM_CLASS; i++)
					{
						if (classes[i] > max_pred)
						{
							pred_class = i;
							max_pred = classes[i];

						}
					}

					/* Store the result into the list if the probability is more than the threshold */
					probability = max_pred * objectness;
					if (probability > TH_PROB)
					{
						d = {bb, pred_class, probability};
						det.push_back(d);
					}
				}
			}
		}
	}
    return ;
}

void YoloInference::read_imagefile( std::string filename ) {
	this->img.read_bmp(filename);
}
void YoloInference::write_imagefile( std::string filename ) {
	this->img.save_bmp(filename);
}
/*****************************************
* Function Name : render
* Description   : Function to printout details of single bounding box to standard output
* Arguments     : d = detected box details
*                 i = result number
* Return value  : -
******************************************/
void YoloInference::render( void ) {

	for ( int i = 0; i < this->det.size(); i++ ) {
		this->img.draw_rect((int32_t)this->det[i].bbox.x, (int32_t)this->det[i].bbox.y,
				(int32_t)this->det[i].bbox.w, (int32_t)this->det[i].bbox.h, label_file_map[det[i].c].c_str() );
	}
}
/*****************************************
* Function Name : print_result_yolo
* Description   : Function to printout details of single bounding box to standard output
* Arguments     : d = detected box details
*                 i = result number
* Return value  : -
******************************************/
void YoloInference::print_result_yolo( void )
{
	int32_t i;
	detection *d;

	for (i = 0;i < det.size(); i++)
	{
		/* Skip the overlapped bounding boxes */
		if (det[i].prob == 0) continue;

		printf("Result %d -----------------------------------------*\n", i);
		printf("\x1b[1m");
		printf("Class           : %s\n",label_file_map[det[i].c].c_str());
		printf("\x1b[0m");
#if 1
		d = &det[i];
		printf("(X, Y, W, H)    : (%d, %d, %d, %d)\n",
			(int32_t) d->bbox.x, (int32_t) d->bbox.y, (int32_t) d->bbox.w, (int32_t) d->bbox.h);
#endif
		printf("Probability     : %.1f %%\n\n",  det[i].prob*100);
	}
    return;
}
void YoloInference::print_numResutls( void )
{
	if ( det.size() > 0 )
		printf("Number of Detections    : %d\n", (int)det.size() );
}
float* YoloInference::getOutputBuf(void)
{
    return &drpai_output_buf[0];
}

/*****************************************
* Function Name : get_result
* Description   : Get DRP-AI Output from memory via DRP-AI Driver
* Arguments     : drpai_fd = file descriptor of DRP-AI Driver
*                 output_addr = memory start address of DRP-AI output
*                 output_size = output data size
* Return value  : 0 if succeeded
*                 not 0 otherwise
******************************************/
int8_t YoloInference::get_result( void )
{
    float drpai_buf[BUF_SIZE];
    int32_t i = 0;
    int8_t ret = 0;

    errno = 0;
    /* Assign the memory address and size to be read */
    ret = ioctl(drpai_fd, DRPAI_ASSIGN, &drpai_outdata);
    if (-1 == ret)
    {
        fprintf(stderr, "[ERROR] Failed to run DRPAI_ASSIGN: errno=%d\n", errno);
        return -1;
    }

    /* Read the memory via DRP-AI Driver and store the output to buffer */
    for (i = 0; i < (drpai_outdata.size/BUF_SIZE); i++)
    {
        errno = 0;
        ret = read(drpai_fd, drpai_buf, BUF_SIZE);
        if ( -1 == ret )
        {
            fprintf(stderr, "[ERROR] Failed to read via DRP-AI Driver: errno=%d\n", errno);
            return -1;
        }

        memcpy(&drpai_output_buf[BUF_SIZE/sizeof(float)*i], drpai_buf, BUF_SIZE);
    }
    /* Get that last few bytes */
    if ( 0 != (drpai_outdata.size % BUF_SIZE))
    {
        errno = 0;
        ret = read(drpai_fd, drpai_buf, (drpai_outdata.size % BUF_SIZE));
        if ( -1 == ret)
        {
            fprintf(stderr, "[ERROR] Failed to read via DRP-AI Driver: errno=%d\n", errno);
            return -1;
        }

        memcpy(&drpai_output_buf[(drpai_outdata.size - (drpai_outdata.size%BUF_SIZE))/sizeof(float)], drpai_buf, (drpai_outdata.size % BUF_SIZE));
    }
    return 0;
}



/*****************************************
* Function Name : yolo_offset
* Description   : Get the offset nuber to access the bounding box attributes
*                 To get the actual value of bounding box attributes, use yolo_index() after this function.
* Arguments     : n = output layer number [0~2].
*                 b = Number to indicate which bounding box in the region [0~2]
*                 y = Number to indicate which region [0~13]
*                 x = Number to indicate which region [0~13]
* Return value  : offset to access the bounding box attributes.
******************************************/
int32_t yolo_offset(uint8_t n, int32_t b, int32_t y, int32_t x)
{
    uint8_t num = num_grids[n];
    uint32_t prev_layer_num = 0;
    int32_t i = 0;

    for (i = 0 ; i < n; i++)
    {
        prev_layer_num += NUM_BB *(NUM_CLASS + 5)* num_grids[i] * num_grids[i];
    }
    return prev_layer_num + b *(NUM_CLASS + 5)* num * num + y * num + x;
}

/*****************************************
* Function Name : yolo_index
* Description   : Get the index of the bounding box attributes based on the input offset.
* Arguments     : n = output layer number.
*                 offs = offset to access the bounding box attributesd.
*                 channel = channel to access each bounding box attribute.
* Return value  : index to access the bounding box attribute.
******************************************/
int32_t yolo_index(uint8_t n, int32_t offs, int32_t channel)
{
    uint8_t num_grid = num_grids[n];
    return offs + channel * num_grid * num_grid;
}

/*****************************************
* Function Name : sigmoid
* Description   : Helper function for YOLO Post Processing
* Arguments     : x = input argument for the calculation
* Return value  : sigmoid result of input x
******************************************/
double sigmoid(double x)
{
    return 1.0/(1.0 + exp(-x));
}
/*****************************************
* Function Name : softmax
* Description   : Helper function for YOLO Post Processing
* Arguments     : val[] = array to be computed softmax
* Return value  : -
******************************************/
void softmax(float val[NUM_CLASS])
{
    float max_num = -FLT_MAX;
    float sum = 0;
    int32_t i;
    for ( i = 0 ; i<NUM_CLASS ; i++ )
    {
        max_num = max(max_num, val[i]);
    }

    for ( i = 0 ; i<NUM_CLASS ; i++ )
    {
        val[i]= (float) exp(val[i] - max_num);
        sum+= val[i];
    }

    for ( i = 0 ; i<NUM_CLASS ; i++ )
    {
        val[i]= val[i]/sum;
    }
    return;
}
