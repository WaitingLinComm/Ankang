// without while iteration

//#include <iostream>
//using namespace std;
//#include <vector>
//#include "tmp_cfo_compen.h"

void CFOPHOTO_estimator_test1(vector<double> &v_I, vector<double> &v_Q, int Lv, vector<double> &q_I, vector<double> &q_Q, int Lq, int K, double Ts)
{   
    //Parameter setting
    int cfo_estimate = 0;
    int cfo_step = 1000;
    int nSteps = 10;
    int iterations = 0;
    
    ///////////////// set plan /////////////////
    fftw_plan p1;
    fftw_plan p2;
    fftw_plan p3;

    /// set data used in iteration
    vector<double>  q_flip_I, q_flip_Q;
    int L_q = Lq;

    vector<double>  Sig_I, Sig_Q;
    int L_r = Lv;

    int Lz = L_r + L_q - 1;     //L_r + L_q - 1; Do Lz points fft
    int number_of_counts = 7;
    
    //////////////////// set q /////////////////////////////////
    fftw_complex *q, *q_fft;
    q = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * Lz);
    q_fft = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * Lz);
    
    ///////////////// set r /////////////////////////////////
    fftw_complex *r, *r_fft;
    r = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * Lz);
    r_fft = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * Lz);

    ///////////////// set z /////////////////////////////////
    fftw_complex *z1, *z1_ifft_conv;
    z1 = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * Lz);
    z1_ifft_conv = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * Lz);


    ////////////////////////  realize r_fft = fft(r, Lz)   ////////////////////////
    vector<double> r_fft_I_vec, r_fft_Q_vec;
    r_fft_I_vec.reserve(Lz); r_fft_Q_vec.reserve(Lz);

    for(int i = 0; i < Lz; i++){  
        r[i][0] = v_I[i]; r[i][1] = v_Q[i];
    }

    p2 = fftw_plan_dft_1d(Lz, r, r_fft, FFTW_FORWARD, FFTW_ESTIMATE);// | FFTW_PRESERVE_INPUT);
    fftw_execute(p2);
    
    for(int i = 0; i < Lz; i++){  
        r_fft_I_vec.push_back(r_fft[i][0]); r_fft_Q_vec.push_back(r_fft[i][1]); 
    }
    //test_printf_2_vec(r_fft_I_vec, r_fft_Q_vec,  20);

   /////////// start iteration //////////////
    iterations = iterations + 1;
    int L_vCFO = 2 * nSteps + 1; // length of vCFO vector
    vector<double> vCFO;
    vCFO.reserve(L_vCFO); 
    for (int i = 0; i < L_vCFO; i++){
        vCFO.push_back(cfo_estimate + (-nSteps + i) * cfo_step); // Hz 
    }
    //test_printf_1_vec(vCFO, 10); (OK)

    vector<double> y2_I, y2_Q;      // for 迴圈中再reserve y2 記憶體大小: y2_I.reserve(L_v), y2_Q.reserve(L_v); 
    vector<double> y2_norm2_buffer;
    y2_norm2_buffer.reserve(Lv);
    
    vector<vector<double> > y2_norm2(L_vCFO, vector<double>(Lv));  // 定義二維動態陣列 y2_norm2[L_vCFO][Lv]
    vector<vector<double> > pho(L_vCFO, vector<double>(Lv));       // 定義二維動態陣列 pho[L_vCFO][Lv]
    vector<double> CFO;
    CFO.reserve(L_vCFO); 

    // set buffer
    vector<double> to_estimate_buffer, cfo_estimate_buffer, pho_estimate_buffer;
    to_estimate_buffer.reserve(100);
    cfo_estimate_buffer.reserve(100);
    pho_estimate_buffer.reserve(100);

    //for (int i = 1; i <= 3; i++) //
    for (int i = 1; i <= L_vCFO; i++)
    {
        //cout << i << "\n";
        vector<double> p_I, p_Q;
        int Lp = Lq;
        p_I.reserve(Lp); p_Q.reserve(Lp); 
        CFO.push_back(2 * pi * vCFO[i-1] * Ts); //CFO:[-pi,pi] (rad);
        tmp_cfo_compen(q_I, q_Q, p_I, p_Q, CFO[i-1], Lq);
 
        ///// realize y2 = filter(conj(p(end : -1 : 1)), 1, v); % or y2 = fftconv((v),conj(p(end:-1:1))); ////
        // (OK)realize : q(end : -1 : 1)
        vector<double>  p_flip_I, p_flip_Q;
        p_flip_I.reserve(Lp); p_flip_Q.reserve(Lp); 
        Flip_vec(p_flip_I, p_I, Lp);
        Flip_vec(p_flip_Q, p_Q, Lp);

        ////////////////       realize : y1 = filter(conj(q(end : -1 : 1)), 1, r)     /////////////////////
        for(int i = 0; i < L_q; i++){    
            q[i][0] = p_flip_I[i]; q[i][1] = - p_flip_Q[i]; // conj(q(end : -1 : 1)
        }
        p_flip_I.clear(); p_flip_Q.clear(); //conj_p_flip_Q.clear();  
        p1 = fftw_plan_dft_1d(Lz, q, q_fft, FFTW_FORWARD, FFTW_ESTIMATE);// |FFTW_PRESERVE_INPUT);

        fftw_execute(p1);   
        
        // (OK)
        /*
        for (int i = 0; i < 20; i++){
            printf("[%d]: %f + j*%f\n", i, q_fft[i][0], q_fft[i][1]);
        } 
        */
        ////////////////////////  realize r_fft = fft(r, Lz)   ////////////////////////    
        for(int i = 0; i < L_r; i++){  
            r[i][0] = v_I[i]; r[i][1] = v_Q[i];
        }      
        p2 = fftw_plan_dft_1d(Lz, r, r_fft, FFTW_FORWARD, FFTW_ESTIMATE);// | FFTW_PRESERVE_INPUT);
        fftw_execute(p2);
        ////////////////////////  realize z1_ifft = fft(r, Lz) * fft(q, Lz)   ////////////////////////
        for (int i = 0; i < Lz; i++){
            z1[i][0] = r_fft_I_vec[i] * q_fft[i][0] - r_fft_Q_vec[i] * q_fft[i][1]; 
            z1[i][1] = r_fft_I_vec[i] * q_fft[i][1] + r_fft_Q_vec[i] * q_fft[i][0]; 
        }
        
        p3 = fftw_plan_dft_1d(Lz, z1, z1_ifft_conv, FFTW_BACKWARD, FFTW_ESTIMATE);//FFTW_PRESERVE_INPUT);
        
        fftw_execute(p3); //repeat as needed 

        vector<double> y2_I, y2_Q;
        y2_I.reserve(Lv); y2_Q.reserve(Lv);  

        for (int i = 0; i < Lv; i++) {
            y2_I.push_back(z1_ifft_conv[i][0] * 1./Lz);
            y2_Q.push_back(z1_ifft_conv[i][1] * 1./Lz);
        }
        
        ////////// calculate norm ///////////////////
        for(int j = 0; j < Lv; j++){ 
            y2_norm2[i - 1][j] = pow(y2_I[j],2) + pow(y2_Q[j],2);
        }
        
        for(int j = 0; j < Lv; j++){ 
            pho[i - 1][j] = atan2(y2_Q[j], y2_I[j]); // atan(Y,X)
        }
    }   
    fftw_destroy_plan(p1);
    fftw_free(q);  fftw_free(q_fft);
    fftw_free(r);  fftw_free(r_fft);
    fftw_free(z1); fftw_free(z1_ifft_conv);
    
    // printf 2D array
/*    
    for(int i=0; i< 3; i++)
    //for(int i=0; i< y2_norm2.size(); i++)//输出二维动态数组 
    //for(int i=0; i< L_vCFO; i++)
    {
        for(int j = 0; j < Lv; j++)
        {
            
            printf("[%d,%d]: %f ", i, j, pho[i][j]);
            //cout<<y2_norm2[i][j]<<" ";
        }
        cout<<"\n";
    }
*/    
    //////////  find the corresponding index of the peak ///////////////////
    int cfo_estimate_id, to_estimate_id;
    double max = pow(10, -100);
    for(int i=0; i< L_vCFO; i++)
    {
        for(int j = 0; j < Lv; j++)
        {
            if(y2_norm2[i][j] > max){
                max = y2_norm2[i][j];
                cfo_estimate_id = i;
                to_estimate_id = j;
            }
        }
    }
    double pho_tmp;
    pho_tmp = pho[cfo_estimate_id][to_estimate_id];

    cfo_step = cfo_step/nSteps * 2;
    
    //cout << cfo_estimate_id << "\n";
    //cout << to_estimate_id << "\n";

    cfo_estimate_buffer.push_back(cfo_estimate_id);
    to_estimate_buffer.push_back(to_estimate_id);
    pho_estimate_buffer.push_back(pho_tmp);
    //cout << pho_tmp << "\n";

}