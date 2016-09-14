
#include "cv.h"  
#include "highgui.h"  

int main()
{
	cvcapture *capture = cvcreatefilecapture("d:\\calib\\1080p.mp4");
	iplimage *mframe, *current, *frg, *test;
	int *fg, *bg_bw, *rank_ind;
	double *w, *mean, *sd, *u_diff, *rank;
	int c, m, sd_init, i, j, k, m, rand_temp = 0, rank_ind_temp = 0, min_index = 0, x = 0, y = 0, counter_frame = 0;
	double d, alph, thresh, p, temp;
	cvrng state;
	int match, height, width;
	mframe = cvqueryframe(capture);

	frg = cvcreateimage(cvsize(mframe->width, mframe->height), ipl_depth_8u, 1);
	current = cvcreateimage(cvsize(mframe->width, mframe->height), ipl_depth_8u, 1);
	test = cvcreateimage(cvsize(mframe->width, mframe->height), ipl_depth_8u, 1);

	c = 4;                      //number of gaussian components (typically 3-5)  
	m = 4;                      //number of background components  
	sd_init = 6;                //initial standard deviation (for new components) var = 36 in paper  
	alph = 0.01;                //learning rate (between 0 and 1) (from paper 0.01)  
	d = 2.5;                    //positive deviation threshold  
	thresh = 0.25;              //foreground threshold (0.25 or 0.75 in paper)  
	p = alph / (1 / c);         //initial p variable (used to update mean and sd)  

	height = current->height; width = current->widthstep;

	fg = (int *)malloc(sizeof(int)*width*height);                   //foreground array  
	bg_bw = (int *)malloc(sizeof(int)*width*height);                //background array  
	rank = (double *)malloc(sizeof(double) * 1 * c);                //rank of components (w/sd)  
	w = (double *)malloc(sizeof(double)*width*height*c);            //weights array  
	mean = (double *)malloc(sizeof(double)*width*height*c);         //pixel means  
	sd = (double *)malloc(sizeof(double)*width*height*c);           //pixel standard deviations  
	u_diff = (double *)malloc(sizeof(double)*width*height*c);       //difference of each pixel from mean  

	for (i = 0; i<height; i++)
	{
		for (j = 0; j<width; j++)
		{
			for (k = 0; k<c; k++)
			{
				mean[i*width*c + j*c + k] = cvrandreal(&state) * 255;
				w[i*width*c + j*c + k] = (double)1 / c;
				sd[i*width*c + j*c + k] = sd_init;
			}
		}
	}

	while (1) {
		rank_ind = (int *)malloc(sizeof(int)*c);
		cvcvtcolor(mframe, current, cv_bgr2gray);
		// calculate difference of pixel values from mean  
		for (i = 0; i<height; i++)
		{
			for (j = 0; j<width; j++)
			{
				for (m = 0; m<c; m++)
				{
					u_diff[i*width*c + j*c + m] = abs((uchar)current->imagedata[i*width + j] - mean[i*width*c + j*c + m]);
				}
			}
		}
		//update gaussian components for each pixel  
		for (i = 0; i<height; i++)
		{
			for (j = 0; j<width; j++)
			{
				match = 0;
				temp = 0;
				for (k = 0; k<c; k++)
				{
					if (abs(u_diff[i*width*c + j*c + k]) <= d*sd[i*width*c + j*c + k])      //pixel matches component  
					{
						match = 1;                                                 // variable to signal component match  

																				   //update weights, mean, sd, p  
						w[i*width*c + j*c + k] = (1 - alph)*w[i*width*c + j*c + k] + alph;
						p = alph / w[i*width*c + j*c + k];
						mean[i*width*c + j*c + k] = (1 - p)*mean[i*width*c + j*c + k] + p*(uchar)current->imagedata[i*width + j];
						sd[i*width*c + j*c + k] = sqrt((1 - p)*(sd[i*width*c + j*c + k] * sd[i*width*c + j*c + k]) + p*(pow((uchar)current->imagedata[i*width + j] - mean[i*width*c + j*c + k], 2)));
					}
					else {
						w[i*width*c + j*c + k] = (1 - alph)*w[i*width*c + j*c + k];           // weight slighly decreases  
					}
					temp += w[i*width*c + j*c + k];
				}

				for (k = 0; k<c; k++)
				{
					w[i*width*c + j*c + k] = w[i*width*c + j*c + k] / temp;
				}

				temp = w[i*width*c + j*c];
				bg_bw[i*width + j] = 0;
				for (k = 0; k<c; k++)
				{
					bg_bw[i*width + j] = bg_bw[i*width + j] + mean[i*width*c + j*c + k] * w[i*width*c + j*c + k];
					if (w[i*width*c + j*c + k] <= temp)
					{
						min_index = k;
						temp = w[i*width*c + j*c + k];
					}
					rank_ind[k] = k;
				}

				test->imagedata[i*width + j] = (uchar)bg_bw[i*width + j];

				//if no components match, create new component  
				if (match == 0)
				{
					mean[i*width*c + j*c + min_index] = (uchar)current->imagedata[i*width + j];
					//printf("%d ",(uchar)bg->imagedata[i*width+j]);  
					sd[i*width*c + j*c + min_index] = sd_init;
				}
				for (k = 0; k<c; k++)
				{
					rank[k] = w[i*width*c + j*c + k] / sd[i*width*c + j*c + k];
					//printf("%f ",w[i*width*c+j*c+k]);  
				}

				//sort rank values  
				for (k = 1; k<c; k++)
				{
					for (m = 0; m<k; m++)
					{
						if (rank[k] > rank[m])
						{
							//swap max values  
							rand_temp = rank[m];
							rank[m] = rank[k];
							rank[k] = rand_temp;

							//swap max index values  
							rank_ind_temp = rank_ind[m];
							rank_ind[m] = rank_ind[k];
							rank_ind[k] = rank_ind_temp;
						}
					}
				}

				//calculate foreground  
				match = 0; k = 0;
				//frg->imagedata[i*width+j]=0;  
				while ((match == 0) && (k<m)) {
					if (w[i*width*c + j*c + rank_ind[k]] >= thresh)
						if (abs(u_diff[i*width*c + j*c + rank_ind[k]]) <= d*sd[i*width*c + j*c + rank_ind[k]]) {
							frg->imagedata[i*width + j] = 0;
							match = 1;
						}
						else
							//frg->imagedata[i*width + j] = (uchar)current->imagedata[i*width + j];
							frg->imagedata[i*width + j] = 255;
					k = k + 1;
				}
			}
		}

		mframe = cvqueryframe(capture);
		cvshowimage("fore", frg);
		cvshowimage("back", test);
		char s = cvwaitkey(33);
		if (s == 27) break;
		free(rank_ind);
	}

	free(fg); free(w); free(mean); free(sd); free(u_diff); free(rank);
	cvnamedwindow("back", 0);
	cvnamedwindow("fore", 0);
	cvreleasecapture(&capture);
	cvdestroywindow("fore");
	cvdestroywindow("back");
	return 0;
}