/*
 * Copyright 2019 Gianluca Frison, Dimitris Kouzoupis, Robin Verschueren,
 * Andrea Zanelli, Niels van Duijkeren, Jonathan Frey, Tommaso Sartor,
 * Branimir Novoselnik, Rien Quirynen, Rezart Qelibari, Dang Doan,
 * Jonas Koenemann, Yutao Chen, Tobias Schöls, Jonas Schlagenhauf, Moritz Diehl
 *
 * This file is part of acados.
 *
 * The 2-Clause BSD License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.;
 */


/* 
 * Description: linear least-squares (LLS) cost module (ocp_nlp)
 *
 * min_w (Vx*x + Vu*u + Vz*z - yref)^T*W*(Vx*x + Vu*u + Vz*z - yref),
 *
 */

#include "acados/ocp_nlp/ocp_nlp_cost_ls.h"
#include "acados/ocp_nlp/ocp_nlp_cost_common.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// blasfeo
#include "blasfeo/include/blasfeo_d_aux.h"
#include "blasfeo/include/blasfeo_d_blas.h"
// acados
#include "acados/utils/mem.h"



////////////////////////////////////////////////////////////////////////////////
//                                     dims                                   //
////////////////////////////////////////////////////////////////////////////////



int ocp_nlp_cost_ls_dims_calculate_size(void *config_)
{

    
    int size = sizeof(ocp_nlp_cost_ls_dims);

    return size;
}



void *ocp_nlp_cost_ls_dims_assign(void *config_, void *raw_memory)
{
    
    char *c_ptr = (char *) raw_memory;

    ocp_nlp_cost_ls_dims *dims = (ocp_nlp_cost_ls_dims *) c_ptr;
    c_ptr += sizeof(ocp_nlp_cost_ls_dims);

    assert((char *) raw_memory + ocp_nlp_cost_ls_dims_calculate_size(config_) >= c_ptr);

    return dims;
}



void ocp_nlp_cost_ls_dims_initialize(void *config_, void *dims_, int nx,
        int nu, int ny, int ns, int nz)
{

    ///  Initialize the dimensions struct of the 
    ///  ocp_nlp_cost_ls module    
    ///
    ///  \param[in] config_ structure containing configuration of ocp_nlp_cost module 
    ///  \param[in] nx_ number of states 
    ///  \param[in] nu number of inputs
    ///  \param[in] ny number of residuals 
    ///  \param[in] ns number of slacks
    ///  \param[out] dims
    ///  \return [] 
    
    ocp_nlp_cost_ls_dims *dims = dims_;

    dims->nx = nx;
    dims->nu = nu;
    dims->ny = ny;
    dims->ns = ns;
    dims->nz = nz;

    return;
}



static void ocp_nlp_cost_ls_set_nx(void *config_, void *dims_, int *nx)
{
    ///  Initialize the dimensions struct of the 
    ///  ocp_nlp_cost_ls module    
    ///
    ///  \param[in] config_ structure containing configuration of ocp_nlp_cost module 
    ///  \param[in] nx_ number of states 
    ///  \param[in] nu number of inputs
    ///  \param[in] ny number of residuals 
    ///  \param[in] ns number of slacks
    ///  \param[out] dims
    ///  \return [] 

    ocp_nlp_cost_ls_dims *dims = (ocp_nlp_cost_ls_dims *) dims_;
    dims->nx = *nx;
}


static void ocp_nlp_cost_ls_set_nz(void *config_, void *dims_, int *nz)
{
    ocp_nlp_cost_ls_dims *dims = (ocp_nlp_cost_ls_dims *) dims_;
    dims->nz = *nz;
}

static void ocp_nlp_cost_ls_set_nu(void *config_, void *dims_, int *nu)
{
    ocp_nlp_cost_ls_dims *dims = (ocp_nlp_cost_ls_dims *) dims_;
    dims->nu = *nu;
}



static void ocp_nlp_cost_ls_set_ny(void *config_, void *dims_, int *ny)
{
    ocp_nlp_cost_ls_dims *dims = (ocp_nlp_cost_ls_dims *) dims_;
    dims->ny = *ny;
}



static void ocp_nlp_cost_ls_set_ns(void *config_, void *dims_, int *ns)
{
    ocp_nlp_cost_ls_dims *dims = (ocp_nlp_cost_ls_dims *) dims_;
    dims->ns = *ns;
}



void ocp_nlp_cost_ls_dims_set(void *config_, void *dims_, const char *field, int* value)
{
    if (!strcmp(field, "nx"))
    {
        ocp_nlp_cost_ls_set_nx(config_, dims_, value);
    }
    else if (!strcmp(field, "nz"))
    {
        // do nothing
        ocp_nlp_cost_ls_set_nz(config_, dims_, value);
    }
    else if (!strcmp(field, "nu"))
    {
        ocp_nlp_cost_ls_set_nu(config_, dims_, value);
    }
    else if (!strcmp(field, "ny"))
    {
        ocp_nlp_cost_ls_set_ny(config_, dims_, value);
    }
    else if (!strcmp(field, "ns"))
    {
        ocp_nlp_cost_ls_set_ns(config_, dims_, value);
    }
    else
    {
        printf("\nerror: dimension type not available in module\n");
        exit(1);
    }
}



/* dimension getters */
static void ocp_nlp_cost_ls_get_ny(void *config_, void *dims_, int* value)
{
    ocp_nlp_cost_ls_dims *dims = (ocp_nlp_cost_ls_dims *) dims_;
    *value = dims->ny;
}



void ocp_nlp_cost_ls_dims_get(void *config_, void *dims_, const char *field, int* value)
{
    if (!strcmp(field, "ny"))
    {
        ocp_nlp_cost_ls_get_ny(config_, dims_, value);
    }
    else
    {
        printf("error: ocp_nlp_cost_ls_dims_get: attempt to get dimensions of non-existing field %s\n", field);
        exit(1);
    }
}
////////////////////////////////////////////////////////////////////////////////
//                                     model                                  //
////////////////////////////////////////////////////////////////////////////////



int ocp_nlp_cost_ls_model_calculate_size(void *config_, void *dims_)
{
    ocp_nlp_cost_ls_dims *dims = dims_;

    // extract dims
    int nx = dims->nx;
    int nz = dims->nz;
    int nu = dims->nu;
    int ny = dims->ny;
    int ns = dims->ns;

    int size = 0;

    size += sizeof(ocp_nlp_cost_ls_model);

    size += 1 * 64;  // blasfeo_mem align

    size += 1 * blasfeo_memsize_dmat(ny, ny);           // W
    size += 1 * blasfeo_memsize_dmat(nu + nx, ny);      // Cyt
    size += 1 * blasfeo_memsize_dmat(nz, ny);           // Vz
    size += 1 * blasfeo_memsize_dvec(ny);               // y_ref
    size += 2 * blasfeo_memsize_dvec(2 * ns);           // Z, z

    return size;
}



void *ocp_nlp_cost_ls_model_assign(void *config_, void *dims_, void *raw_memory)
{
    ocp_nlp_cost_ls_dims *dims = dims_;

    char *c_ptr = (char *) raw_memory;

    int nx = dims->nx;
    int nz = dims->nz;
    int nu = dims->nu;
    int ny = dims->ny;
    int ns = dims->ns;

    // struct
    ocp_nlp_cost_ls_model *model = (ocp_nlp_cost_ls_model *) c_ptr;
    c_ptr += sizeof(ocp_nlp_cost_ls_model);

    // blasfeo_mem align
    align_char_to(64, &c_ptr);

    // blasfeo_dmat
    // W
    assign_and_advance_blasfeo_dmat_mem(ny, ny, &model->W, &c_ptr);
    // Cyt
    assign_and_advance_blasfeo_dmat_mem(nu + nx, ny, &model->Cyt, &c_ptr);

    // Vz
    assign_and_advance_blasfeo_dmat_mem(nz, ny, &model->Vz, &c_ptr);

    // blasfeo_dvec
    // y_ref
    assign_and_advance_blasfeo_dvec_mem(ny, &model->y_ref, &c_ptr);
    // Z
    assign_and_advance_blasfeo_dvec_mem(2 * ns, &model->Z, &c_ptr);
    // z
    assign_and_advance_blasfeo_dvec_mem(2 * ns, &model->z, &c_ptr);

    // default initialization
    model->scaling = 1.0;

    // assert
    assert((char *) raw_memory + 
        ocp_nlp_cost_ls_model_calculate_size(config_, dims) >= c_ptr);

    return model;
}




int ocp_nlp_cost_ls_model_set(void *config_, void *dims_, void *model_,
                                 const char *field, void *value_)
{
    int status = ACADOS_SUCCESS;

    if ( !config_ || !dims_ || !model_ || !value_ )
    {
        printf("ocp_nlp_cost_ls_model_set: got NULL pointer \n");
        exit(1);
    }

    ocp_nlp_cost_ls_dims *dims = dims_;
    ocp_nlp_cost_ls_model *model = model_;

    int nx = dims->nx;
    int nu = dims->nu;
    int ny = dims->ny;
    int ns = dims->ns;

    if (!strcmp(field, "W"))
    {
        double *W_col_maj = (double *) value_;
        blasfeo_pack_dmat(ny, ny, W_col_maj, ny, &model->W, 0, 0);
    }
    else if (!strcmp(field, "Cyt"))
    {
        double *Cyt_col_maj = (double *) value_;
        blasfeo_pack_dmat(nx + nu, dims->ny, Cyt_col_maj, nx + nu, 
            &model->Cyt, 0, 0);
    }
    else if (!strcmp(field, "Vx"))
    {
        double *Vx_col_maj = (double *) value_;
        blasfeo_pack_tran_dmat(ny, nx, Vx_col_maj, ny, &model->Cyt, nu, 0);
    }
    else if (!strcmp(field, "Vu"))
    {
        double *Vu_col_maj = (double *) value_;
        blasfeo_pack_tran_dmat(ny, nu, Vu_col_maj, ny, &model->Cyt, 0, 0);
    }
    // TODO(andrea): inconsistent order x, u, z. Make x, z, u later!
    else if (!strcmp(field, "Vz"))
    {
        double *Vz_col_maj = (double *) value_;
        blasfeo_pack_dmat(dims->ny, dims->nz, Vz_col_maj, dims->ny,
                &model->Vz, 0, 0);
    }
    else if (!strcmp(field, "y_ref") || !strcmp(field, "yref"))
    {
        double *y_ref = (double *) value_;
        blasfeo_pack_dvec(ny, y_ref, &model->y_ref, 0);
    }
    else if (!strcmp(field, "Z"))
    {
        double *Z = (double *) value_;
        blasfeo_pack_dvec(ns, Z, &model->Z, 0);
        blasfeo_pack_dvec(ns, Z, &model->Z, ns);
    }
    else if (!strcmp(field, "Zl"))
    {
        double *Zl = (double *) value_;
        blasfeo_pack_dvec(ns, Zl, &model->Z, 0);
    }
    else if (!strcmp(field, "Zu"))
    {
        double *Zu = (double *) value_;
        blasfeo_pack_dvec(ns, Zu, &model->Z, ns);
    }
    else if (!strcmp(field, "z"))
    {
        double *z = (double *) value_;
        blasfeo_pack_dvec(ns, z, &model->z, 0);
        blasfeo_pack_dvec(ns, z, &model->z, ns);
    }
    else if (!strcmp(field, "zl"))
    {
        double *zl = (double *) value_;
        blasfeo_pack_dvec(ns, zl, &model->z, 0);
    }
    else if (!strcmp(field, "zu"))
    {
        double *zu = (double *) value_;
        blasfeo_pack_dvec(ns, zu, &model->z, ns);
    }
    else if (!strcmp(field, "scaling"))
    {
        double *scaling_ptr = (double *) value_;
        model->scaling = *scaling_ptr;
    }
    else
    {
        printf("\nerror: field %s not available in ocp_nlp_cost_ls_model_set\n", field);
        
        exit(1);
    }
    return status;
}



////////////////////////////////////////////////////////////////////////////////
//                                   options                                  //
////////////////////////////////////////////////////////////////////////////////



int ocp_nlp_cost_ls_opts_calculate_size(void *config_, void *dims_)
{
    int size = 0;

    size += sizeof(ocp_nlp_cost_ls_opts);

    return size;
}



void *ocp_nlp_cost_ls_opts_assign(void *config_, void *dims_, void *raw_memory)
{
    char *c_ptr = (char *) raw_memory;

    ocp_nlp_cost_ls_opts *opts = (ocp_nlp_cost_ls_opts *) c_ptr;
    c_ptr += sizeof(ocp_nlp_cost_ls_opts);

    assert((char *) raw_memory + 
        ocp_nlp_cost_ls_opts_calculate_size(config_, dims_) >= c_ptr);

    return opts;
}



void ocp_nlp_cost_ls_opts_initialize_default(void *config_, 
    void *dims_, void *opts_)
{
    // ocp_nlp_cost_ls_opts *opts = opts_;

    return;
}



void ocp_nlp_cost_ls_opts_update(void *config_, void *dims_, void *opts_)
{
    // ocp_nlp_cost_ls_opts *opts = opts_;

    return;
}



void ocp_nlp_cost_ls_opts_set(void *config_, void *opts_, const char *field, void* value)
{
    // ocp_nlp_cost_config *config = config_;
    // ocp_nlp_cost_ls_opts *opts = opts_;

    if(!strcmp(field, "exact_hess"))
    {
        // do nothing: the exact hessian is always computed
    }
    else
    {
        printf("\nerror: field %s not available in ocp_nlp_cost_ls_opts_set\n", field);
        exit(1);
    }

    return;

}



////////////////////////////////////////////////////////////////////////////////
//                                     memory                                 //
////////////////////////////////////////////////////////////////////////////////



int ocp_nlp_cost_ls_memory_calculate_size(void *config_, 
    void *dims_, void *opts_)
{
    ocp_nlp_cost_ls_dims *dims = dims_;

    // extract dims
    int nx = dims->nx;
    int nu = dims->nu;
    int ny = dims->ny;
    int ns = dims->ns;

    int size = 0;

    size += sizeof(ocp_nlp_cost_ls_memory);

    size += 1 * blasfeo_memsize_dmat(nu + nx, nu + nx);  // hess
    size += 1 * blasfeo_memsize_dmat(ny, ny);            // W_chol
    size += 1 * blasfeo_memsize_dvec(ny);                // res
    size += 1 * blasfeo_memsize_dvec(nu + nx + 2 * ns);  // grad

    size += 1 * 64;  // blasfeo_mem align

    return size;
}



void *ocp_nlp_cost_ls_memory_assign(void *config_, void *dims_, void *opts_, 
    void *raw_memory)
{
    ocp_nlp_cost_ls_dims *dims = dims_;

    char *c_ptr = (char *) raw_memory;

    // extract dims
    int nx = dims->nx;
    int nu = dims->nu;
    int ny = dims->ny;
    int ns = dims->ns;

    // struct
    ocp_nlp_cost_ls_memory *memory = (ocp_nlp_cost_ls_memory *) c_ptr;
    c_ptr += sizeof(ocp_nlp_cost_ls_memory);

    // blasfeo_mem align
    align_char_to(64, &c_ptr);

    // hess
    assign_and_advance_blasfeo_dmat_mem(nu + nx, nu + nx, &memory->hess, &c_ptr);
    // W_chol
    assign_and_advance_blasfeo_dmat_mem(ny, ny, &memory->W_chol, &c_ptr);
    // res
    assign_and_advance_blasfeo_dvec_mem(ny, &memory->res, &c_ptr);
    // grad
    assign_and_advance_blasfeo_dvec_mem(nu + nx + 2 * ns, &memory->grad, &c_ptr);

    assert((char *) raw_memory + 
        ocp_nlp_cost_ls_memory_calculate_size(config_, dims, opts_) >= c_ptr);

    return memory;
}



struct blasfeo_dvec *ocp_nlp_cost_ls_memory_get_grad_ptr(void *memory_)
{
    ocp_nlp_cost_ls_memory *memory = memory_;

    return &memory->grad;
}



void ocp_nlp_cost_ls_memory_set_RSQrq_ptr(struct 
    blasfeo_dmat *RSQrq, void *memory_)
{
    ocp_nlp_cost_ls_memory *memory = memory_;

    memory->RSQrq = RSQrq;
}



void ocp_nlp_cost_ls_memory_set_Z_ptr(struct blasfeo_dvec *Z, void *memory_)
{
    ocp_nlp_cost_ls_memory *memory = memory_;

    memory->Z = Z;
}



void ocp_nlp_cost_ls_memory_set_ux_ptr(struct blasfeo_dvec *ux, void *memory_)
{
    ocp_nlp_cost_ls_memory *memory = memory_;

    memory->ux = ux;
}



void ocp_nlp_cost_ls_memory_set_z_alg_ptr(struct blasfeo_dvec *z_alg, void *memory_)
{
    ocp_nlp_cost_ls_memory *memory = memory_;

    memory->z_alg = z_alg;
}



void ocp_nlp_cost_ls_memory_set_dzdux_tran_ptr(struct blasfeo_dmat *dzdux_tran, void *memory_)
{
    ocp_nlp_cost_ls_memory *memory = memory_;

    memory->dzdux_tran = dzdux_tran;
}



////////////////////////////////////////////////////////////////////////////////
//                                 workspace                                  //
////////////////////////////////////////////////////////////////////////////////



int ocp_nlp_cost_ls_workspace_calculate_size(void *config_, 
    void *dims_, void *opts_)
{
    ocp_nlp_cost_ls_dims *dims = dims_;

    // extract dims
    int nx = dims->nx;
    int nu = dims->nu;
    int ny = dims->ny;
    int nz = dims->nz;

    int size = 0;

    size += sizeof(ocp_nlp_cost_ls_workspace);

    size += 1 * blasfeo_memsize_dmat(nu + nx, ny);  // tmp_nv_ny
    size += 1 * blasfeo_memsize_dmat(nu + nx, ny);  // Cyt_tilde
    size += 1 * blasfeo_memsize_dmat(nu + nx, nz);  // dzdux_tran
    size += 1 * blasfeo_memsize_dvec(ny);           // tmp_ny
    size += 1 * blasfeo_memsize_dvec(nz);           // tmp_nz
    size += 1 * blasfeo_memsize_dvec(ny);           // y_ref_tilde

    size += 1 * 64;  // blasfeo_mem align

    return size;
}



static void ocp_nlp_cost_ls_cast_workspace(void *config_, 
    void *dims_, void *opts_, void *work_)
{
    ocp_nlp_cost_ls_dims *dims = dims_;
    ocp_nlp_cost_ls_workspace *work = work_;

    // extract dims
    int nx = dims->nx;
    int nu = dims->nu;
    int ny = dims->ny;
    int nz = dims->nz;

    char *c_ptr = (char *) work_;
    c_ptr += sizeof(ocp_nlp_cost_ls_workspace);

    // blasfeo_mem align
    align_char_to(64, &c_ptr);

    // tmp_nv_ny
    assign_and_advance_blasfeo_dmat_mem(nu + nx, ny, &work->tmp_nv_ny, &c_ptr);

    // Cyt_tilde
    assign_and_advance_blasfeo_dmat_mem(nu + nx, ny, &work->Cyt_tilde, &c_ptr);

    // dzdux_tran
    assign_and_advance_blasfeo_dmat_mem(nu + nx, nz, &work->dzdux_tran, &c_ptr);

    // tmp_ny
    assign_and_advance_blasfeo_dvec_mem(ny, &work->tmp_ny, &c_ptr);

    // tmp_nz
    assign_and_advance_blasfeo_dvec_mem(nz, &work->tmp_nz, &c_ptr);

    // y_ref_tilde
    assign_and_advance_blasfeo_dvec_mem(ny, &work->y_ref_tilde, &c_ptr);

    assert((char *) work + ocp_nlp_cost_ls_workspace_calculate_size(config_, dims, opts_) >= c_ptr);

    return;
}



////////////////////////////////////////////////////////////////////////////////
//                                 functions                                  //
////////////////////////////////////////////////////////////////////////////////



// TODO move computataion of hess into pre-compute???
void ocp_nlp_cost_ls_initialize(void *config_, void *dims_, void *model_, 
    void *opts_, void *memory_, void *work_)
{
    ocp_nlp_cost_ls_dims *dims = dims_;
    ocp_nlp_cost_ls_model *model = model_;
    // ocp_nlp_cost_ls_opts *opts = opts_;
    ocp_nlp_cost_ls_memory *memory = memory_;
    ocp_nlp_cost_ls_workspace *work = work_;

    ocp_nlp_cost_ls_cast_workspace(config_, dims, opts_, work_);

    int nx = dims->nx;
    int nu = dims->nu;
    int ny = dims->ny;
    int ns = dims->ns;

    // general Cyt

    // TODO(all): recompute factorization only if W are re-tuned ???
    blasfeo_dpotrf_l(ny, &model->W, 0, 0, &memory->W_chol, 0, 0);

    // TODO(all): avoid recomputing the Hessian if both W and Cyt do not change
    blasfeo_dtrmm_rlnn(nu + nx, ny, 1.0, &memory->W_chol, 0, 0, &model->Cyt,
        0, 0, &work->tmp_nv_ny,
                       0, 0);
    blasfeo_dsyrk_ln(nu+nx, ny, model->scaling, &work->tmp_nv_ny, 0, 0,
        &work->tmp_nv_ny, 0, 0, 0.0, &memory->hess, 0, 0, &memory->hess, 0, 0);

    blasfeo_dveccpsc(2*ns, model->scaling, &model->Z, 0, memory->Z, 0);

    return;
}



void ocp_nlp_cost_ls_update_qp_matrices(void *config_, void *dims_,
    void *model_, void *opts_, void *memory_, void *work_)
{
    ocp_nlp_cost_ls_dims *dims = dims_;
    ocp_nlp_cost_ls_model *model = model_;
    // ocp_nlp_cost_ls_opts *opts = opts_;
    ocp_nlp_cost_ls_memory *memory = memory_;
    ocp_nlp_cost_ls_workspace *work = work_;

    ocp_nlp_cost_ls_cast_workspace(config_, dims, opts_, work_);

    int nx = dims->nx;
    int nu = dims->nu;
    int nz = dims->nz;
    int ny = dims->ny;
    int ns = dims->ns;

    if (nz > 0)
    { // eliminate algebraic variables and update Cyt and y_ref
        // copy Cyt into Cyt_tilde
        blasfeo_dgecp(nu + nx, ny, &model->Cyt, 0, 0, &work->Cyt_tilde, 0, 0);
        // copy y_ref into y_ref_tilde
        blasfeo_dveccp(ny, &model->y_ref, 0, &work->y_ref_tilde, 0);

        // update Cyt: Cyt_tilde = Cyt + dzdux_tran*Vz^T
        blasfeo_dgemm_nt(nu + nx, ny, nz, 1.0, memory->dzdux_tran, 0, 0,
                &model->Vz, 0, 0, 1.0, &work->Cyt_tilde, 0, 0, &work->Cyt_tilde, 0, 0);

        // update y_ref: y_ref_tilde = y_ref + Vz*dzdx*x + Vz*dzdu*u - Vz*z
        blasfeo_dgemv_t(nx + nu, nz, -1.0, memory->dzdux_tran,
                0, 0, memory->ux, 0, 1.0, memory->z_alg, 0, &work->tmp_nz, 0);

        blasfeo_dgemv_n(ny, nz, -1.0, &model->Vz,
                0, 0, &work->tmp_nz, 0, 1.0, &work->y_ref_tilde, 0, &work->y_ref_tilde, 0);

        blasfeo_dtrmm_rlnn(nu + nx, ny, 1.0, &memory->W_chol, 0, 0, &work->Cyt_tilde, 0, 0, &work->tmp_nv_ny, 0, 0);

        // add hessian of the cost contribution
        blasfeo_dsyrk_ln(nu + nx, ny, model->scaling, &work->tmp_nv_ny, 0, 0, &work->tmp_nv_ny, 0, 0, 1.0,
                memory->RSQrq, 0, 0, memory->RSQrq, 0, 0);

        // compute gradient
        blasfeo_dgemv_t(nu + nx, ny, 1.0, &work->Cyt_tilde, 0, 0, memory->ux,
                0, -1.0, &work->y_ref_tilde, 0, &memory->res, 0);

        blasfeo_dsymv_l(ny, ny, 1.0, &model->W, 0, 0, &memory->res,
                0, 0.0, &work->tmp_ny, 0, &work->tmp_ny, 0);

        blasfeo_dgemv_n(nu + nx, ny, 1.0, &work->Cyt_tilde,
                0, 0, &work->tmp_ny, 0, 0.0, &memory->grad, 0, &memory->grad, 0);

		// TODO what about the exact hessian in the case of nz>0 ?????????????????????????????????????
    }
    else
    {
        // add hessian of the cost contribution
        blasfeo_dgead(nx + nu, nx + nu, 1.0, &memory->hess, 0, 0, memory->RSQrq, 0, 0);
        // // initialize hessian of lagrangian with hessian of cost
        // blasfeo_dgecp(nu + nx, nu + nx, &memory->hess, 0, 0, memory->RSQrq, 0, 0);

        // compute gradient
        blasfeo_dgemv_t(nu + nx, ny, 1.0, &model->Cyt, 0, 0, memory->ux,
                0, -1.0, &model->y_ref, 0, &memory->res, 0);

        blasfeo_dsymv_l(ny, ny, 1.0, &model->W, 0, 0, &memory->res,
                0, 0.0, &work->tmp_ny, 0, &work->tmp_ny, 0);

        blasfeo_dgemv_n(nu + nx, ny, 1.0, &model->Cyt,
                0, 0, &work->tmp_ny, 0, 0.0, &memory->grad, 0, &memory->grad, 0);
    }

    // slacks
    blasfeo_dveccp(2*ns, &model->z, 0, &memory->grad, nu+nx);
    blasfeo_dvecmulacc(2*ns, &model->Z, 0, memory->ux,
        nu+nx, &memory->grad, nu+nx);

    // scale
    if(model->scaling!=1.0)
    {
        blasfeo_dvecsc(nu+nx+2*ns, model->scaling, &memory->grad, 0);
    }

    return;
}



void ocp_nlp_cost_ls_config_initialize_default(void *config_)
{
    ocp_nlp_cost_config *config = config_;

    config->dims_calculate_size = &ocp_nlp_cost_ls_dims_calculate_size;
    config->dims_assign = &ocp_nlp_cost_ls_dims_assign;
    config->dims_initialize = &ocp_nlp_cost_ls_dims_initialize;
    config->dims_set = &ocp_nlp_cost_ls_dims_set;
    config->dims_get = &ocp_nlp_cost_ls_dims_get;
    config->model_calculate_size = &ocp_nlp_cost_ls_model_calculate_size;
    config->model_assign = &ocp_nlp_cost_ls_model_assign;
    config->model_set = &ocp_nlp_cost_ls_model_set;
    config->opts_calculate_size = &ocp_nlp_cost_ls_opts_calculate_size;
    config->opts_assign = &ocp_nlp_cost_ls_opts_assign;
    config->opts_initialize_default = &ocp_nlp_cost_ls_opts_initialize_default;
    config->opts_update = &ocp_nlp_cost_ls_opts_update;
    config->opts_set = &ocp_nlp_cost_ls_opts_set;
    config->memory_calculate_size = &ocp_nlp_cost_ls_memory_calculate_size;
    config->memory_assign = &ocp_nlp_cost_ls_memory_assign;
    config->memory_get_grad_ptr = &ocp_nlp_cost_ls_memory_get_grad_ptr;
    config->memory_set_ux_ptr = &ocp_nlp_cost_ls_memory_set_ux_ptr;
    config->memory_set_z_alg_ptr = &ocp_nlp_cost_ls_memory_set_z_alg_ptr;
    config->memory_set_dzdux_tran_ptr = &ocp_nlp_cost_ls_memory_set_dzdux_tran_ptr;
    config->memory_set_RSQrq_ptr = &ocp_nlp_cost_ls_memory_set_RSQrq_ptr;
    config->memory_set_Z_ptr = &ocp_nlp_cost_ls_memory_set_Z_ptr;
    config->workspace_calculate_size = &ocp_nlp_cost_ls_workspace_calculate_size;
    config->initialize = &ocp_nlp_cost_ls_initialize;
    config->update_qp_matrices = &ocp_nlp_cost_ls_update_qp_matrices;
    config->config_initialize_default = &ocp_nlp_cost_ls_config_initialize_default;

    return;
}
