#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "inlet_component_defs.h"
#include "struct.h"
#include "func.h"
#include "os/os.h"

static int find(int x, int* parent) {
    if (parent[x] != x)
        parent[x] = find(parent[x], parent);
    return parent[x];
}

static void unite(int x, int y, int* parent) {
    int fx = find(x, parent);
    int fy = find(y, parent);
    if (fx != fy)
        parent[fx] = fy;
}

static int inlet_components_are_pure_gas(struct inlet_component_data *comps)
{
    const type eps = 1e-12;
    type total_fraction = 0.0;
    type gas_fraction = 0.0;
    int gas_phase_index = -1;

    if (!comps)
        return OFF;

    for (int ic = 0; ic < comps->ncomp; ++ic) {
        struct inlet_component_element *element = &comps->array[ic];
        type ratio;

        ratio = element->ratio.current_value;
        total_fraction += ratio;

        if (element->comp.id == gas_phase_index) {
            gas_fraction += ratio;
        }
    }

    if (total_fraction <= 0.0)
        return OFF;

    return fabs(total_fraction - gas_fraction) <= eps;
}

static int system_boundary_is_inlet(struct fluid_boundary_data *boundary)
{
    return boundary && boundary->cond == INLET &&
           inlet_components_are_pure_gas(boundary->comps);
}

static int surface_boundary_is_inlet(struct surface_boundary_data *boundary)
{
    return boundary && boundary->cond == INLET &&
           inlet_components_are_pure_gas(boundary->comps);
}

static int touches_inlet_boundary(variable *val, parameter *prm,
                                  int j, int jx, int jy, int jz);

static int system_boundary_face_is_inlet(variable *val, parameter *prm,
                                         int face, int boundary_x,
                                         int boundary_y, int boundary_z)
{
    domain *cdo = prm->cdo;
    ptrdiff_t p;

    switch (face) {
    case 4:
        p = boundary_y + (ptrdiff_t)cdo->nby * boundary_z;
        return system_boundary_is_inlet(val->bnd_W.fl[p]);
    case 5:
        p = boundary_y + (ptrdiff_t)cdo->nby * boundary_z;
        return system_boundary_is_inlet(val->bnd_E.fl[p]);
    case 2:
        p = boundary_x + (ptrdiff_t)cdo->nbx * boundary_z;
        return system_boundary_is_inlet(val->bnd_S.fl[p]);
    case 3:
        p = boundary_x + (ptrdiff_t)cdo->nbx * boundary_z;
        return system_boundary_is_inlet(val->bnd_N.fl[p]);
    case 0:
        p = boundary_x + (ptrdiff_t)cdo->nbx * boundary_y;
        return system_boundary_is_inlet(val->bnd_B.fl[p]);
    case 1:
        p = boundary_x + (ptrdiff_t)cdo->nbx * boundary_y;
        return system_boundary_is_inlet(val->bnd_T.fl[p]);
    default:
        return OFF;
    }
}

static void get_core_scan_bounds(parameter *prm,
                                 int *xs, int *xe,
                                 int *ys, int *ye,
                                 int *zs, int *ze)
{
    domain *cdo = prm->cdo;

    *xs = cdo->stm;
    *xe = cdo->nx + cdo->stm;
    *ys = cdo->stm;
    *ye = cdo->ny + cdo->stm;
    *zs = cdo->stm;
    *ze = cdo->nz + cdo->stm;
}

static void get_label_scan_bounds(parameter *prm,
                                  int *xs, int *xe,
                                  int *ys, int *ye,
                                  int *zs, int *ze)
{
    domain *cdo = prm->cdo;
    mpi_param *mpi = prm->mpi;

    get_core_scan_bounds(prm, xs, xe, ys, ye, zs, ze);

    if (mpi->nrk[4] == -1)
        *xs = cdo->stm - 1;
    if (mpi->nrk[5] == -1)
        *xe = cdo->nx + cdo->stm + 1;
    if (mpi->nrk[2] == -1)
        *ys = cdo->stm - 1;
    if (mpi->nrk[3] == -1)
        *ye = cdo->ny + cdo->stm + 1;
    if (mpi->nrk[0] == -1)
        *zs = cdo->stm - 1;
    if (mpi->nrk[1] == -1)
        *ze = cdo->nz + cdo->stm + 1;
}

static void get_peripheral_scan_bounds(parameter *prm,
                                       int *xs, int *xe,
                                       int *ys, int *ye,
                                       int *zs, int *ze)
{
    domain *cdo = prm->cdo;
    mpi_param *mpi = prm->mpi;

    get_core_scan_bounds(prm, xs, xe, ys, ye, zs, ze);

    if (mpi->nrk[4] == -1)
        *xs = 0;
    if (mpi->nrk[5] == -1)
        *xe = cdo->mx;
    if (mpi->nrk[2] == -1)
        *ys = 0;
    if (mpi->nrk[3] == -1)
        *ye = cdo->my;
    if (mpi->nrk[0] == -1)
        *zs = 0;
    if (mpi->nrk[1] == -1)
        *ze = cdo->mz;
}

static int coords_are_owned_cell(parameter *prm, int jx, int jy, int jz)
{
    domain *cdo = prm->cdo;

    return jx >= cdo->stm && jx < cdo->stm + cdo->nx &&
           jy >= cdo->stm && jy < cdo->stm + cdo->ny &&
           jz >= cdo->stm && jz < cdo->stm + cdo->nz;
}

static int cell_is_physical_boundary_orifice_ghost(variable *val, parameter *prm,
                                                   int jx, int jy, int jz)
{
    domain *cdo = prm->cdo;
    mpi_param *mpi = prm->mpi;

    if (jy >= cdo->stm && jy < cdo->stm + cdo->ny &&
        jz >= cdo->stm && jz < cdo->stm + cdo->nz) {
        int boundary_y = jy - cdo->stm + cdo->stmb;
        int boundary_z = jz - cdo->stm + cdo->stmb;

        if (mpi->nrk[4] == -1 && jx == cdo->stm - 1 &&
            system_boundary_face_is_inlet(val, prm, 4, 0, boundary_y, boundary_z))
            return ON;
        if (mpi->nrk[5] == -1 && jx == cdo->stm + cdo->nx &&
            system_boundary_face_is_inlet(val, prm, 5, 0, boundary_y, boundary_z))
            return ON;
    }

    if (jx >= cdo->stm && jx < cdo->stm + cdo->nx &&
        jz >= cdo->stm && jz < cdo->stm + cdo->nz) {
        int boundary_x = jx - cdo->stm + cdo->stmb;
        int boundary_z = jz - cdo->stm + cdo->stmb;

        if (mpi->nrk[2] == -1 && jy == cdo->stm - 1 &&
            system_boundary_face_is_inlet(val, prm, 2, boundary_x, 0, boundary_z))
            return ON;
        if (mpi->nrk[3] == -1 && jy == cdo->stm + cdo->ny &&
            system_boundary_face_is_inlet(val, prm, 3, boundary_x, 0, boundary_z))
            return ON;
    }

    if (jx >= cdo->stm && jx < cdo->stm + cdo->nx &&
        jy >= cdo->stm && jy < cdo->stm + cdo->ny) {
        int boundary_x = jx - cdo->stm + cdo->stmb;
        int boundary_y = jy - cdo->stm + cdo->stmb;

        if (mpi->nrk[0] == -1 && jz == cdo->stm - 1 &&
            system_boundary_face_is_inlet(val, prm, 0, boundary_x, boundary_y, 0))
            return ON;
        if (mpi->nrk[1] == -1 && jz == cdo->stm + cdo->nz &&
            system_boundary_face_is_inlet(val, prm, 1, boundary_x, boundary_y, 0))
            return ON;
    }

    return OFF;
}

static int cell_is_geometric_orifice(variable *val, parameter *prm,
                                     int j, int jx, int jy, int jz)
{
    type EPS = 1e-5;

    if (coords_are_owned_cell(prm, jx, jy, jz)) {
        if (val->fs_sum[j] > EPS &&
            touches_inlet_boundary(val, prm, j, jx, jy, jz))
            return ON;
    } else if (cell_is_physical_boundary_orifice_ghost(val, prm, jx, jy, jz)) {
        return ON;
    }

    return OFF;
}

static int cell_is_bubble_core(type fl, type fs_sum)
{
    const type threshold = 0.5;
    const type EPS = 1e-5;

    return fs_sum <= EPS && fl < threshold;
}

static int cell_is_liquid_cell(type fl, type fs_sum)
{
    const type threshold = 0.5;
    const type EPS = 1e-5;

    return fl > threshold && fs_sum <= EPS;
}

static void initialize_orifice_layer_state(variable *val, parameter *prm)
{
    domain *cdo = prm->cdo;
    int NumberOfLayer = cdo->NumberOfLayer;
    int m = cdo->m;
    int mx = cdo->mx;
    int mxy = cdo->mxy;
    int xs, xe, ys, ye, zs, ze;
    int j, jx, jy, jz;

    zero_clear_int(val->is_orifice_layer, NumberOfLayer * m);
    get_label_scan_bounds(prm, &xs, &xe, &ys, &ye, &zs, &ze);

    for (jz = zs; jz < ze; jz++) {
        for (jy = ys; jy < ye; jy++) {
            for (jx = xs; jx < xe; jx++) {
                j = jx + mx * jy + mxy * jz;
                if (cell_is_geometric_orifice(val, prm, j, jx, jy, jz)) {
                    val->is_orifice_layer[j] = ON;
                }
            }
        }
    }

    val->is_orifice_layer_initialized = ON;
}

static int touches_inlet_boundary(variable *val, parameter *prm,
                                  int j, int jx, int jy, int jz)
{
    domain *cdo = prm->cdo;
    mpi_param *mpi = prm->mpi;
    int mx = cdo->mx;
    int mxy = cdo->mxy;

    if (jx < cdo->stm || jx >= cdo->stm + cdo->nx ||
        jy < cdo->stm || jy >= cdo->stm + cdo->ny ||
        jz < cdo->stm || jz >= cdo->stm + cdo->nz) {
        return OFF;
    }

    int boundary_x = jx - cdo->stm + cdo->stmb;
    int boundary_y = jy - cdo->stm + cdo->stmb;
    int boundary_z = jz - cdo->stm + cdo->stmb;

    if (val->surface_bnd) {
        struct surface_boundary_data *spu, *spv, *spw;

        spu = val->surface_bnd[3 * j          ]; if(surface_boundary_is_inlet(spu)) return ON;
        spu = val->surface_bnd[3 * (j+1)      ]; if(surface_boundary_is_inlet(spu)) return ON;

        spv = val->surface_bnd[3 * j +       1]; if(surface_boundary_is_inlet(spv)) return ON;
        spv = val->surface_bnd[3 * (j+mx) +  1]; if(surface_boundary_is_inlet(spv)) return ON;

        spw = val->surface_bnd[3 * j +       2]; if(surface_boundary_is_inlet(spw)) return ON;
        spw = val->surface_bnd[3 * (j+mxy) + 2]; if(surface_boundary_is_inlet(spw)) return ON;
    }

    if (mpi->nrk[4] == -1 && jx == cdo->stm) {
        if (system_boundary_face_is_inlet(val, prm, 4, 0, boundary_y, boundary_z)) return ON;
    }
    if (mpi->nrk[5] == -1 && jx == cdo->stm + cdo->nx - 1) {
        if (system_boundary_face_is_inlet(val, prm, 5, 0, boundary_y, boundary_z)) return ON;
    }

    if (mpi->nrk[2] == -1 && jy == cdo->stm) {
        if (system_boundary_face_is_inlet(val, prm, 2, boundary_x, 0, boundary_z)) return ON;
    }
    if (mpi->nrk[3] == -1 && jy == cdo->stm + cdo->ny - 1) {
        if (system_boundary_face_is_inlet(val, prm, 3, boundary_x, 0, boundary_z)) return ON;
    }

    if (mpi->nrk[0] == -1 && jz == cdo->stm) {
        if (system_boundary_face_is_inlet(val, prm, 0, boundary_x, boundary_y, 0)) return ON;
    }
    if (mpi->nrk[1] == -1 && jz == cdo->stm + cdo->nz - 1) {
        if (system_boundary_face_is_inlet(val, prm, 1, boundary_x, boundary_y, 0)) return ON;
    }

    return OFF;
}

#ifdef JUPITER_MPI
static void sync_label_halo(int *label, parameter *prm)
{
    domain *cdo = prm->cdo;
    mpi_param *mpi = prm->mpi;

    int ptr[7], i;
    int mx = cdo->mx, my = cdo->my, mz = cdo->mz;
    int nx = cdo->nx, ny = cdo->ny, nz = cdo->nz;
    int stcl = cdo->stm;

    MPI_Request req_send[6], req_recv[6];
    MPI_Status stat_send[6], stat_recv[6];

    size_t size = sizeof(int) * ((mx * my + my * mz + mz * mx) * 2 * stcl);
    int *sbuff = malloc(size);
    int *rbuff = malloc(size);

    if (sbuff == NULL || rbuff == NULL) {
        printf("Memory allocation failed for halo label buffers\n");
        exit(1);
    }

    calc_ptr(ptr, mx, my, mz, stcl, mpi);

    pack_mpi_z_int(label, sbuff, ptr, nx, ny, nz, mx, my, mz, stcl, mpi);
    mpi_isend_irecv_z_int(sbuff, rbuff, ptr, mx, my, mz, stcl, req_send, req_recv, mpi);
    for (i = 0; i < 2; i++) {
        if (mpi->nrk[i] > -1) {
            MPI_Wait(&req_send[i], &stat_send[i]);
            MPI_Wait(&req_recv[i], &stat_recv[i]);
        }
    }

    pack_mpi_y_int(label, sbuff, ptr, nx, ny, nz, mx, my, mz, stcl, mpi);
    mpi_isend_irecv_y_int(sbuff, rbuff, ptr, mx, my, mz, stcl, req_send, req_recv, mpi);
    for (i = 2; i < 4; i++) {
        if (mpi->nrk[i] > -1) {
            MPI_Wait(&req_send[i], &stat_send[i]);
            MPI_Wait(&req_recv[i], &stat_recv[i]);
        }
    }

    pack_mpi_x_int(label, sbuff, ptr, nx, ny, nz, mx, my, mz, stcl, mpi);
    mpi_isend_irecv_x_int(sbuff, rbuff, ptr, mx, my, mz, stcl, req_send, req_recv, mpi);
    for (i = 4; i < 6; i++) {
        if (mpi->nrk[i] > -1) {
            MPI_Wait(&req_send[i], &stat_send[i]);
            MPI_Wait(&req_recv[i], &stat_recv[i]);
        }
    }

    unpack_mpi_int(label, rbuff, ptr, nx, ny, nz, mx, my, mz, stcl, mpi);

    free(sbuff);
    free(rbuff);
}
#endif

static void expand_label_peripheral(int *label, type *fl,
                                    int *is_orifice_layer, type *fs_sum,
                                    parameter *prm)
{
    domain *cdo = prm->cdo;
    const type threshold = 0.5;

    int j, jx, jy, jz,
        mx = cdo->mx, my = cdo->my, mz = cdo->mz, mxy = cdo->mxy, m = cdo->m;
    int xs, xe, ys, ye, zs, ze;

    get_peripheral_scan_bounds(prm, &xs, &xe, &ys, &ye, &zs, &ze);

    int *tmp_label = malloc(sizeof(int) * m);
    if (tmp_label == NULL) {
        printf("Memory allocation failed for peripheral label array\n");
        exit(1);
    }
    zero_clear_int(tmp_label, m);

/* +1 Peripheral region*/
#pragma omp parallel for private(jz, jy, jx, j)
    for(jz=zs; jz<ze; jz++){
        for(jy=ys; jy<ye; jy++){
            for(jx=xs; jx<xe; jx++){

                j=jx+mx*jy+mxy*jz;

                if (label[j] != 0) {

                    tmp_label[j] = label[j];

                }else if (label[j] == 0) {

                    int chosen_label = 0;
                    type min_fl = 1.0;
                    int target_is_liquid = cell_is_liquid_cell(fl[j], fs_sum[j]);

                    if (!target_is_liquid && is_orifice_layer[j] == ON) {
                        continue;
                    }

                    // 3x3x3 neighborhood search
                    for(int dz=-1; dz<=1; dz++){
                        int nzj = jz + dz;
                        if(nzj < 0 || nzj >= mz) continue;
                        for(int dy=-1; dy<=1; dy++){
                            int nyj = jy + dy;
                            if(nyj < 0 || nyj >= my) continue;
                            for(int dx=-1; dx<=1; dx++){
                                int nxj = jx + dx;
                                if(nxj < 0 || nxj >= mx) continue;
                                if(dx==0 && dy==0 && dz==0) continue; // itself

                                int nj = nxj + mx*nyj + mxy*nzj;

                                int lbl_neighbor = label[nj];
                                int neighbor_is_bubble_core =
                                    lbl_neighbor > 0 &&
                                    cell_is_bubble_core(fl[nj], fs_sum[nj]);
                                int neighbor_is_orifice_core =
                                    lbl_neighbor > 0 &&
                                    is_orifice_layer[nj] == ON;
                                int allow_neighbor =
                                    (target_is_liquid && neighbor_is_bubble_core) ||
                                    neighbor_is_orifice_core;

                                if(allow_neighbor && fl[nj] < min_fl){
                                    chosen_label = lbl_neighbor;
                                    min_fl = fl[nj];
                                }
                            }
                        }
                    }

                    if(chosen_label > 0) tmp_label[j] = chosen_label;
                }
            }
        }
    }

/* +1 Peripheral region*/
#pragma omp parallel for private(jz, jy, jx, j)
    for(jz=zs; jz<ze; jz++){
        for(jy=ys; jy<ye; jy++){
            for(jx=xs; jx<xe; jx++){

                j=jx+mx*jy+mxy*jz;

                label[j]=tmp_label[j];

            }
        }
    }

    free(tmp_label);
}

static int CCL_local(variable *val, parameter *prm, int ilayer){

    domain *cdo=prm->cdo;
    int j, jx, jy, jz, 
        mx=cdo->mx, my=cdo->my, mz=cdo->mz, mxy=cdo->mxy, m=cdo->m;

    int *label = &val->label_layer[ilayer*m]; zero_clear_int(label, m);
    int *is_orifice_layer = &val->is_orifice_layer[ilayer*m];
    type *fl = &val->fl_layer[ilayer*m];
    type *fs_sum = val->fs_sum;

    int possible_maximum_label = cdo->m + 1;

    int *parent = malloc(possible_maximum_label * sizeof(int));
    int *label_map = malloc(possible_maximum_label * sizeof(int));

    if (parent == NULL || label_map == NULL) {
        printf("Memory allocation failed for label arrays\n");
        exit(1);
    }

    int next_label = 1;

    // initialization
    for (int i = 0; i < possible_maximum_label; i++) {
        parent[i] = i;
        label_map[i] = 0;
    }

    int xs, xe, ys, ye, zs, ze;

    get_label_scan_bounds(prm, &xs, &xe, &ys, &ye, &zs, &ze);

    for(jz=zs; jz<ze; jz++){
        for(jy=ys; jy<ye; jy++){
            for(jx=xs; jx<xe; jx++){

                j=jx+mx*jy+mxy*jz;

                int is_orifice = is_orifice_layer[j] == ON;

                //　気泡と判定する条件
                int is_bubble = cell_is_bubble_core(fl[j], fs_sum[j]);

                if (!(is_bubble || is_orifice)) continue; 

                // 過去側13近傍の (dx,dy,dz)
                int neighbor_offsets[13][3] = {
                    {-1,  0,  0},   // west
                    { 0, -1,  0},   // south
                    { 0,  0, -1},   // bottom

                    {-1, -1,  0},   // southwest
                    {+1, -1,  0},   // southeast

                    {-1,  0, -1},   // west-bottom
                    { 0, -1, -1},   // south-bottom
                    {+1,  0, -1},   // east-bottom

                    {-1, -1, -1},   // southwest-bottom
                    { 0, -1, -1},   // south-bottom
                    {+1, -1, -1},   // southeast-bottom
                    {-1, +1, -1},   // northwest-bottom
                    {+1, +1, -1}    // northeast-bottom
                };

                int neighbor_labels[13];
                int ncount = 0;

                // 近傍ラベル収集
                for(int k=0; k<13; k++){
                    int nx = jx + neighbor_offsets[k][0];
                    int ny = jy + neighbor_offsets[k][1];
                    int nz = jz + neighbor_offsets[k][2];

                    if(nx < xs || nx >= xe ||
                       ny < ys || ny >= ye ||
                       nz < zs || nz >= ze) continue;

                    int nj = nx + mx*ny + mxy*nz;

                    if(label[nj] != 0)
                        neighbor_labels[ncount++] = label[nj];
                }

                // --- 最小ラベル検索 ---
                int min_label = 0;
                for(int k=0; k<ncount; k++){
                    if(min_label == 0 || neighbor_labels[k] < min_label)
                        min_label = neighbor_labels[k];
                }

                if(min_label == 0){

                    //if(next_label==1) printf("[DEBUG] The first labeled cell index in this rank is (%d, %d, %d) from (%d, %d)x(%d, %d)x(%d, %d)\n", jx, jy, jz, xs, xe, ys, ye, zs, ze);

                    // No neighboring labels -> assign a new label
                    label[j] = next_label++;

                } else {

                    // Assign the minimum label
                    label[j] = min_label;

                    // Union with neighboring labels
                    for(int k=0; k<ncount; k++){
                        if(neighbor_labels[k] != min_label)
                            unite(neighbor_labels[k], min_label, parent);
                    }
                }

            }

        }
    }



/* Union Find */
// Normalization of labels and replacing them with new sequential labels
    int current_label = 1;
//#pragma omp parallel for private(jz, jy, jx, j)
    for(jz=zs; jz<ze; jz++){
        for(jy=ys; jy<ye; jy++){
            for(jx=xs; jx<xe; jx++){

                j=jx+mx*jy+mxy*jz;

                if (label[j] != 0) {
                    int root = find(label[j], parent);

                    // One idea of labelling is to just do like "label[j]=find(label[j], parent)".
                    // But then the resulted label is not sequential, like (2,2,2,2,0,0,0,5,5,5,0,0,0,9,9,9)
                    // To organize them in sequential manner like (1,1,1,1,0,0,0,2,2,2,0,0,0,3,3,3), each "parent" is given sequential number, and then this substitude in label[j].
                    if (label_map[root] == 0) {
                        label_map[root] = current_label++;
                        //("label update at x y z = %f, %f, %f and f=%f in rank=%d \n", cdo->x[jx], cdo->y[jy], cdo->z[jz], fl[j], mpi->rank);
                    }
                    
                    label[j] = label_map[root];
                }
            }
        }
    }

    free(parent);
    free(label_map);

    return current_label - 1;

}

static int CCL(variable *val, parameter *prm, int ilayer, int last_label_in_lower_layer){

    domain *cdo=prm->cdo;
    mpi_param *mpi = prm->mpi;

    int j, jx, jy, jz,
        mx=cdo->mx, my=cdo->my, mz=cdo->mz, mxy=cdo->mxy, m=cdo->m, stcl=cdo->stm;

    // -----------------------------
    // 1. Perform Connected Component Labeling (CCL) on each rank
    //    using only bubble-core cells. Peripheral expansion is applied
    //    after MPI label reconciliation to avoid false unions.
    // -----------------------------

    int local_max_label = CCL_local(val, prm, ilayer);

#ifdef JUPITER_MPI

    int *label = &val->label_layer[ilayer*m];

    int rank = mpi->rank;
    int npe = mpi->npe; // number of processor element
    int label_xs, label_xe, label_ys, label_ye, label_zs, label_ze;

    get_label_scan_bounds(prm, &label_xs, &label_xe, &label_ys, &label_ye,
                          &label_zs, &label_ze);

    // -----------------------------
    // 2. Avoid label collisions across ranks by assigning rank offsets
    // -----------------------------

    //("local_max_label=%d for rank=%d\n", local_max_label, rank);

    // Gather the maximum label from each rank
    int *max_labels = NULL;
    if(rank==0) max_labels = malloc(sizeof(int)*npe);

    MPI_Gather(&local_max_label, 1, MPI_INT, max_labels, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Determine offsets on rank 0
    int *offset = NULL;
    if (rank == 0) offset = malloc(sizeof(int)*npe);
    int possible_maximum_label = 0;
    if(rank==0){
        offset[0] = 0;
        for(int r=1; r<npe; r++){
            offset[r] = offset[r-1] + max_labels[r-1];
        }
        possible_maximum_label = offset[npe-1] + max_labels[npe-1] + 1;
    }

    // Broadcast possible_maximum_label to all ranks
    MPI_Bcast(&possible_maximum_label, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Scatter offsets to each rank
    int my_offset = 0;
    MPI_Scatter(offset, 1, MPI_INT, &my_offset, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Add the offset to local labels
#pragma omp parallel for private(jz,jy,jx,j)
    for(jz=label_zs; jz<label_ze; jz++){
        for(jy=label_ys; jy<label_ye; jy++){
            for(jx=label_xs; jx<label_xe; jx++){

                j=jx+mx*jy+mxy*jz;

                if(label[j] != 0){
                    label[j] += my_offset;
                }
            }
        }
    }

    sync_label_halo(label, prm);

    // -----------------------------
    // 3. Define parent and label_map (non-rank-0 copies act as placeholders)
    // -----------------------------

    int *parent = NULL; parent = malloc(sizeof(int) * possible_maximum_label);
    int *label_map = NULL; label_map = malloc(sizeof(int) * possible_maximum_label);
    
    for(int i = 0; i < possible_maximum_label; i++){
        parent[i] = i;      // set itself as parent
        label_map[i] = 0;   // sequential labels not assigned yet
    }

    // -----------------------------
    // 4. Perform union-find operations across rank boundaries
    // -----------------------------

    // Collect differing pairs per direction to be gathered at rank 0
    int max_pairs = (mx*my + my*mz + mz*mx)*2*stcl; // worst-case max pairs
    int *local_pairs = malloc(sizeof(int)*2*max_pairs); // local differing pairs
    int num_local_pairs = 0;


    // -----------------------------
    // 4.1 z-direction Union-Find
    // -----------------------------
    if(mpi->nrk[0]>-1){
        for(jz=stcl;jz<2*stcl;jz++){
            for(jy=stcl;jy<cdo->ny+stcl;jy++){
                for(jx=stcl;jx<cdo->nx+stcl;jx++){
                    int halo_jz = jz - stcl;
                    int halo_j = jx + mx*jy + mxy*halo_jz;

                    j=jx+mx*jy+mxy*jz;
                    int lbl = label[j];
                    int nbr_lbl = label[halo_j];

                    if(lbl != 0 && nbr_lbl != 0 && lbl != nbr_lbl){
                        if(num_local_pairs < max_pairs){
                            local_pairs[2*num_local_pairs+0] = lbl;
                            local_pairs[2*num_local_pairs+1] = nbr_lbl;
                            num_local_pairs++;
                        }
                    }
                }
            }
        }
    }

    // -----------------------------
    // 4.2 y-direction union-find 
    // -----------------------------
    if(mpi->nrk[2]>-1){
        for(jz=stcl;jz<cdo->nz+stcl;jz++){
            for(jy=stcl;jy<2*stcl;jy++){
                for(jx=stcl;jx<cdo->nx+stcl;jx++){
                    int halo_jy = jy - stcl;
                    int halo_j = jx + mx*halo_jy + mxy*jz;

                    j=jx+mx*jy+mxy*jz;
                    int lbl = label[j];
                    int nbr_lbl = label[halo_j];

                    if(lbl != 0 && nbr_lbl != 0 && lbl != nbr_lbl){
                        if(num_local_pairs < max_pairs){
                            local_pairs[2*num_local_pairs+0] = lbl;
                            local_pairs[2*num_local_pairs+1] = nbr_lbl;
                            num_local_pairs++;
                        }
                    }
                }
            }
        }
    }

    // -----------------------------
    // 4.3 x-direction Union-Find
    // -----------------------------
    if(mpi->nrk[4]>-1){
        for(jz=stcl;jz<cdo->nz+stcl;jz++){
            for(jy=stcl;jy<cdo->ny+stcl;jy++){
                for(jx=stcl;jx<2*stcl;jx++){
                    int halo_jx = jx - stcl;
                    int halo_j = halo_jx + mx*jy + mxy*jz;

                    j=jx+mx*jy+mxy*jz;
                    int lbl = label[j];
                    int nbr_lbl = label[halo_j];

                    if(lbl != 0 && nbr_lbl != 0 && lbl != nbr_lbl){
                        if(num_local_pairs < max_pairs){
                            local_pairs[2*num_local_pairs+0] = lbl;
                            local_pairs[2*num_local_pairs+1] = nbr_lbl;
                            num_local_pairs++;
                        }
                    }
                }
            }
        }
    }

    // -----------------------------
    // 4.4 Union processing (gather pairs and unite on rank 0)
    // -----------------------------

    // gather to rank 0
    int *all_pairs = NULL;
    int *recvcounts = NULL;
    int *displs = NULL;
    if(rank==0){
        all_pairs = malloc(sizeof(int)*2*max_pairs*npe); // worst-case allocation
        recvcounts = malloc(sizeof(int)*npe);
        displs = malloc(sizeof(int)*npe);
    }

    // send the number of pairs from each process to rank 0
    int num_elements = num_local_pairs*2;
    #ifdef JUPITER_MPI
    MPI_Gather(&num_elements,1,MPI_INT,recvcounts,1,MPI_INT,0,MPI_COMM_WORLD);
    #endif

    // compute displacements for Gatherv
    if(rank==0){
        displs[0] = 0;
        for(int r=1;r<npe;r++){
            displs[r] = displs[r-1]+recvcounts[r-1];
        }
    }

    // gather all pairs at rank 0
    MPI_Gatherv(local_pairs, num_elements, MPI_INT,
                all_pairs, recvcounts, displs, MPI_INT, 0, MPI_COMM_WORLD);

    // perform unite operations on rank 0
    if(rank==0){
        int total_pairs = 0;
        for(int r=0;r<npe;r++) total_pairs += recvcounts[r]/2;

        for(int k=0;k<total_pairs;k++){
            int lbl = all_pairs[2*k];
            int nbr_lbl = all_pairs[2*k+1];
            //printf("lbl=%d, nbr_lbl=%d\n", lbl, nbr_lbl);
            unite(lbl, nbr_lbl, parent);
        }
    }


    // -----------------------------
    // 4.5 Update labels
    // -----------------------------

    // normalize parent array

    int new_label = 0;

    if(rank==0){
        
        for(int i=0;i<possible_maximum_label;i++){
            int root = find(i,parent);
            if(label_map[root]==0) label_map[root] = new_label++;
        }

        //printf("parent ");
        //for(i=0;i<possible_maximum_label;i++){
        //    printf("%d ",parent[i]);
        //}
        //printf("\n");
//
        //printf("label_map ");
        //for(i=0;i<possible_maximum_label;i++){
        //    printf("%d ",label_map[i]);
        //}
        //printf("\n");

        
    }

    MPI_Bcast(&new_label, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Broadcast parent and label_map to all ranks
    MPI_Bcast(parent, possible_maximum_label, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(label_map, possible_maximum_label, MPI_INT, 0, MPI_COMM_WORLD);

    // update labels
#pragma omp parallel for private(jz,jy,jx,j)
    for(jz=label_zs; jz<label_ze; jz++){
        for(jy=label_ys; jy<label_ye; jy++){
            for(jx=label_xs; jx<label_xe; jx++){
                j=jx+mx*jy+mxy*jz;
                if(label[j] != 0){
                    //printf("label=%d, in rank=%d\n", label[j], rank);
                    int root = find(label[j], parent);
                    label[j] = label_map[root];

                    label[j] += last_label_in_lower_layer;
                }
            }
        }
    }

    sync_label_halo(label, prm);

    expand_label_peripheral(label, &val->fl_layer[ilayer*m],
                            &val->is_orifice_layer[ilayer*m], val->fs_sum, prm);
    sync_label_halo(label, prm);

    // -----------------------------
    // 5. Free memory
    // -----------------------------
    free(parent);
    free(label_map);
    free(local_pairs);
    if(rank == 0){
        free(offset);
        free(max_labels);
        free(all_pairs);
        free(recvcounts);
        free(displs);
    }

    return new_label-1;

#else

    expand_label_peripheral(&val->label_layer[ilayer*m], &val->fl_layer[ilayer*m],
                            &val->is_orifice_layer[ilayer*m], val->fs_sum, prm);

    return local_max_label;

#endif

}

typedef struct {
    int label;
    int bbox[6]; // Bounding box: xm, xp, ym, yp, zm, zp
    int volume;    // number of cells (volume)
    int touches_surface_inlet;
    int has_orifice;
} BubbleMeta;

static int touches_surface_inlet_boundary(variable *val, parameter *prm,
                                          int j, int jx, int jy, int jz)
{
    return touches_inlet_boundary(val, prm, j, jx, jy, jz);
}

// Detect bubble contact pairs using a two-stage approach:
// 1) Fast bbox overlap (already expanded by margin k before calling this)
// 2) For provisional pairs, do a stricter local check: if any pair of
//    cells (one from bubble A and one from bubble B) have Chebyshev
//    distance <= 2*k then consider them contacting.
// The check is performed per-rank (local), and the caller is expected to
// gather per-rank results to rank 0 as needed (same pattern used elsewhere).
static void detect_bubble_contact(int *label, BubbleMeta *bubble_meta, int max_label,
                                 int k, int too_small_bubble_volume,
                                 int nx, int ny, int nz,
                                 int mx, int my, int mz, int mxy, int m,
                                 int stm, parameter *prm,
                                 int **contact_pairs_out, int *num_contact_pairs_out)
{
    domain *cdo=prm->cdo;
    // worst-case allocation per rank (keeps behavior similar to existing code)
    int *contact_pairs = malloc(sizeof(int) * 2 * max_label * max_label);
    int num_contact_pairs = 0;

    // First-stage: provisional pairs by bbox overlap (bboxes already expanded by k)
    for(int i=0;i<max_label;i++){
        BubbleMeta *a = &bubble_meta[i];
        int a_is_small_skip =
            a->volume < too_small_bubble_volume &&
            !a->touches_surface_inlet && !a->has_orifice;
        if(a_is_small_skip) continue;

        for(int j=i+1;j<max_label;j++){
            BubbleMeta *b = &bubble_meta[j];
            int b_is_small_skip =
                b->volume < too_small_bubble_volume &&
                !b->touches_surface_inlet && !b->has_orifice;
            if(b_is_small_skip) continue;

            int overlap =
                (a->bbox[0] <= b->bbox[1] && a->bbox[1] >= b->bbox[0]) &&
                (a->bbox[2] <= b->bbox[3] && a->bbox[3] >= b->bbox[2]) &&
                (a->bbox[4] <= b->bbox[5] && a->bbox[5] >= b->bbox[4]);
            if(!overlap) continue;

            // Second-stage (local): restrict search to intersection of expanded bboxes
            int ix0 = fmax(a->bbox[0], b->bbox[0]);
            int ix1 = fmin(a->bbox[1], b->bbox[1]);
            int iy0 = fmax(a->bbox[2], b->bbox[2]);
            int iy1 = fmin(a->bbox[3], b->bbox[3]);
            int iz0 = fmax(a->bbox[4], b->bbox[4]);
            int iz1 = fmin(a->bbox[5], b->bbox[5]);

            if(ix0>ix1 || iy0>iy1 || iz0>iz1) continue;

            // Collect local coordinates using local array indices, including halo cells.
            int a_label = a->label;
            int b_label = b->label;

            // temporary dynamic arrays (expected to be small because region is limited)
            int capA = 256, capB = 256;
            int cntA = 0, cntB = 0;
            int (*coordsA)[3] = malloc(sizeof(int)*3*capA);
            int (*coordsB)[3] = malloc(sizeof(int)*3*capB);

            for(int ilayer = 0; ilayer < cdo->NumberOfLayer; ilayer++){
                for(int iz = iz0; iz <= iz1; iz++){
                    for(int iy = iy0; iy <= iy1; iy++){
                        for(int ix = ix0; ix <= ix1; ix++){
                            int j = ix + mx*iy + mxy*iz;
                            int lbl = label[j + ilayer*cdo->m];

                            if(lbl==0) continue;

                            if(lbl == a_label){
                                if(cntA >= capA){ capA *= 2; coordsA = realloc(coordsA, sizeof(int)*3*capA); }
                                coordsA[cntA][0] = ix; coordsA[cntA][1] = iy; coordsA[cntA][2] = iz;
                                cntA++;
                            } else if(lbl == b_label){
                                if(cntB >= capB){ capB *= 2; coordsB = realloc(coordsB, sizeof(int)*3*capB); }
                                coordsB[cntB][0] = ix; coordsB[cntB][1] = iy; coordsB[cntB][2] = iz;
                                cntB++;
                            }

                        }
                    }
                }                
            }


            int local_confirms = 0;
            if(cntA>0 && cntB>0){
                //int thresh = 2*k;
                int thresh = 3;
                for(int ia=0; ia<cntA && !local_confirms; ia++){
                    for(int ib=0; ib<cntB; ib++){
                        int dx = abs(coordsA[ia][0] - coordsB[ib][0]);
                        int dy = abs(coordsA[ia][1] - coordsB[ib][1]);
                        int dz = abs(coordsA[ia][2] - coordsB[ib][2]);
                        if(dx*dx + dy*dy + dz*dz <= thresh*thresh ){ local_confirms = 1; break; }
                    }
                }
            }

            free(coordsA); free(coordsB);

            if(local_confirms){
                contact_pairs[2*num_contact_pairs+0] = a_label;
                contact_pairs[2*num_contact_pairs+1] = b_label;
                num_contact_pairs++;
            }
        }
    }

    *contact_pairs_out = contact_pairs;
    *num_contact_pairs_out = num_contact_pairs;
}

void define_fl_layer_from_fl(variable *val, parameter *prm){

    // For the multi_layer model assume the liquid phase exists only for property ID=0
    // (i.e. only fl_0 is non-zero, fl_1 and fl_2 are zero). Therefore level-sets
    // should be expanded only for fl_layer entries under fl_0.

  domain *cdo=prm->cdo;

    // 0th layer: use fl_0 provided by geom.txt as-is.
#pragma omp for
    for(int j=0; j<cdo->m; j++){
      val->fl_layer[j]=fmax(val->fl[j]-val->fs_sum[j], 0.0);
    // If the user has excluded liquid in solid regions (fs_1 etc.) then
    // val->fl_layer[j] = val->fl[j] would suffice; but to be safe we use
    // val->fl[j] - val->fs_sum[j].
      val->fls_layer[j]=val->fl_layer[j] + val->fs_sum[j];
    }

    // For layers >= 1: fill bubble regions with liquid. If other material
    // IDs are present in fs, those regions remain void (fl_layer=0).
    //            vv 1 here
    for(int ilayer= 1 ; ilayer<cdo->NumberOfLayer; ilayer++) {
#pragma omp for
      for(int j=0; j<cdo->m; j++){
        val->fl_layer[j + ilayer*cdo->m]=fmax(1.0-val->fs_sum[j], 0.0);
        val->fls_layer[j + ilayer*cdo->m]=1.0;
      }
    }

        initialize_orifice_layer_state(val, prm);

}

static void APT(variable *val, parameter *prm, int max_label){

    domain *cdo=prm->cdo;
    mpi_param *mpi = prm->mpi;

    int too_small_bubble_volume = 100;

    int rank = mpi->rank;
    int npe = mpi->npe; // number of processor element

    int NumberOfLayer = cdo->NumberOfLayer;

    int nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
        mx=cdo->mx, my=cdo->my, mz=cdo->mz, 
        mxy=cdo->mxy, m=cdo->m, stcl=cdo->stm, stm=cdo->stm;

    int *label = val->label_layer;

    int jx, jy, jz ,j, jl, i, ilayer;
    int label_xs, label_xe, label_ys, label_ye, label_zs, label_ze;

    int zs = (mpi->nrk[0] > -1) ? 0 : stcl;
    int ze = (mpi->nrk[1] > -1) ? mz : nz+stcl;
    int ys = (mpi->nrk[2] > -1) ? 0 : stcl;
    int ye = (mpi->nrk[3] > -1) ? my : ny+stcl;
    int xs = (mpi->nrk[4] > -1) ? 0 : stcl;
    int xe = (mpi->nrk[5] > -1) ? mx: nx+stcl;

    get_label_scan_bounds(prm, &label_xs, &label_xe, &label_ys, &label_ye,
                          &label_zs, &label_ze);

    if(prm->mpi->rank == 0 && prm->cdo->viewflg == 1){
//    if(prm->mpi->rank == 0 && (prm->cdo->viewflg == 1 || 1)){ // debug
        printf("[Multi_layer Information]\n");
        printf("-- Number of label is %d\n", max_label);
    }

    // --- Find bounding boxes for each labeled object ---
    BubbleMeta *bubble_meta = malloc(sizeof(BubbleMeta) * max_label);
    for(int lbl=1; lbl<=max_label; lbl++){
        BubbleMeta *bm = &bubble_meta[lbl-1];
        bm->label = lbl;
        bm->bbox[0] = bm->bbox[2] = bm->bbox[4] = INT_MAX;
        bm->bbox[1] = bm->bbox[3] = bm->bbox[5] = INT_MIN;
        bm->volume = 0;
        bm->touches_surface_inlet = 0;
        bm->has_orifice = 0;
    }

#pragma omp parallel for private(ilayer,jz,jy,jx,j,jl)
    for(ilayer=0; ilayer<NumberOfLayer; ilayer++){
        for(jz = 0; jz < nz; jz++) {
            for(jy = 0; jy < ny; jy++) {
                for(jx = 0; jx < nx; jx++) {
                    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
                    jl = j + m*ilayer;
                    int lbl = label[jl];
                    if(lbl > 0 && lbl <= max_label){
                        int is_orifice = val->is_orifice_layer[jl] == ON;

#pragma omp critical
                        {
                            BubbleMeta *bm = &bubble_meta[lbl-1];
                            if(is_orifice) bm->has_orifice = 1;
                            else bm->volume++;
                        }

                    }
                }
            }
        }
    }

#pragma omp parallel for private(ilayer,jz,jy,jx,j,jl)
    for(ilayer=0; ilayer<NumberOfLayer; ilayer++){
        for(jz = label_zs; jz < label_ze; jz++) {
            for(jy = label_ys; jy < label_ye; jy++) {
                for(jx = label_xs; jx < label_xe; jx++) {
                    j = jx + mx*jy + mxy*jz;
                    jl = j + m*ilayer;
                    int lbl = label[jl];
                    if(lbl > 0 && lbl <= max_label){
                        int is_orifice = val->is_orifice_layer[jl] == ON;

#pragma omp critical
                        {
                            BubbleMeta *bm = &bubble_meta[lbl-1];
                            if(jx < bm->bbox[0]) bm->bbox[0] = jx;
                            if(jx > bm->bbox[1]) bm->bbox[1] = jx;
                            if(jy < bm->bbox[2]) bm->bbox[2] = jy;
                            if(jy > bm->bbox[3]) bm->bbox[3] = jy;
                            if(jz < bm->bbox[4]) bm->bbox[4] = jz;
                            if(jz > bm->bbox[5]) bm->bbox[5] = jz;
                            if(is_orifice) bm->has_orifice = 1;
                            if(touches_surface_inlet_boundary(val, prm, j, jx, jy, jz)) bm->touches_surface_inlet = 1;
                        }

                    }
                }
            }
        }
    }

    #ifdef JUPITER_MPI
    int *local_volume = malloc(sizeof(int) * max_label);
    int *global_volume = NULL;
    int *local_touches_surface_inlet = malloc(sizeof(int) * max_label);
    int *global_touches_surface_inlet = NULL;
    int *local_has_orifice = malloc(sizeof(int) * max_label);
    int *global_has_orifice = NULL;
    int *local_bbox_min = malloc(sizeof(int) * 3 * max_label);
    int *local_bbox_max = malloc(sizeof(int) * 3 * max_label);
    int *global_bbox_min = NULL;
    int *global_bbox_max = NULL;
    int *label_remap = malloc(sizeof(int) * max_label);
    for(int i=0;i<max_label;i++) local_volume[i] = bubble_meta[i].volume;
    for(int i=0;i<max_label;i++) local_touches_surface_inlet[i] = bubble_meta[i].touches_surface_inlet;
    for(int i=0;i<max_label;i++) local_has_orifice[i] = bubble_meta[i].has_orifice;
    for(int i=0;i<max_label;i++){
        local_bbox_min[3*i + 0] = bubble_meta[i].bbox[0];
        local_bbox_min[3*i + 1] = bubble_meta[i].bbox[2];
        local_bbox_min[3*i + 2] = bubble_meta[i].bbox[4];
        local_bbox_max[3*i + 0] = bubble_meta[i].bbox[1];
        local_bbox_max[3*i + 1] = bubble_meta[i].bbox[3];
        local_bbox_max[3*i + 2] = bubble_meta[i].bbox[5];
        label_remap[i] = 0;
    }

    if(rank==0) global_volume = malloc(sizeof(int) * max_label);
    if(rank==0) global_touches_surface_inlet = malloc(sizeof(int) * max_label);
    if(rank==0) global_has_orifice = malloc(sizeof(int) * max_label);
    if(rank==0) global_bbox_min = malloc(sizeof(int) * 3 * max_label);
    if(rank==0) global_bbox_max = malloc(sizeof(int) * 3 * max_label);

    // Aggregate bubble metadata on rank 0. Labels with zero global owned volume
    // are MPI-orphan labels that only survived in halos, so compact them away.
    MPI_Reduce(local_volume, global_volume, max_label, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(local_touches_surface_inlet, global_touches_surface_inlet, max_label, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(local_has_orifice, global_has_orifice, max_label, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(local_bbox_min, global_bbox_min, 3 * max_label, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(local_bbox_max, global_bbox_max, 3 * max_label, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

    int compact_max_label = max_label;
    if(rank==0){
        int next_label = 0;
        for(int i=0;i<max_label;i++) {
            bubble_meta[i].volume = global_volume[i];
            bubble_meta[i].touches_surface_inlet = global_touches_surface_inlet[i];
            bubble_meta[i].has_orifice = global_has_orifice[i];
            bubble_meta[i].bbox[0] = global_bbox_min[3*i + 0];
            bubble_meta[i].bbox[2] = global_bbox_min[3*i + 1];
            bubble_meta[i].bbox[4] = global_bbox_min[3*i + 2];
            bubble_meta[i].bbox[1] = global_bbox_max[3*i + 0];
            bubble_meta[i].bbox[3] = global_bbox_max[3*i + 1];
            bubble_meta[i].bbox[5] = global_bbox_max[3*i + 2];

            if(bubble_meta[i].volume > 0 || bubble_meta[i].has_orifice){
                label_remap[i] = ++next_label;
                if(next_label - 1 != i){
                    bubble_meta[next_label - 1] = bubble_meta[i];
                }
                bubble_meta[next_label - 1].label = next_label;
            }
        }
        compact_max_label = next_label;
    }

    MPI_Bcast(&compact_max_label, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(label_remap, max_label, MPI_INT, 0, MPI_COMM_WORLD);
    if(compact_max_label > 0){
        MPI_Bcast(bubble_meta, compact_max_label * (int)sizeof(BubbleMeta), MPI_BYTE, 0, MPI_COMM_WORLD);
    }

#pragma omp parallel for
    for(int jl=0; jl<NumberOfLayer*m; jl++){
        int lbl = label[jl];
        if(lbl > 0 && lbl <= max_label){
            label[jl] = label_remap[lbl-1];
        }
    }

    max_label = compact_max_label;

    free(local_volume);
    free(local_touches_surface_inlet);
    free(local_has_orifice);
    free(local_bbox_min);
    free(local_bbox_max);
    free(label_remap);
    if(rank==0) free(global_volume);
    if(rank==0) free(global_touches_surface_inlet);
    if(rank==0) free(global_has_orifice);
    if(rank==0) free(global_bbox_min);
    if(rank==0) free(global_bbox_max);
    #endif

    //--- Add ±k cell margin to bounding boxes ---
    // k is the contact margin (in cells). Default k=2, overridable via
    // environment variable JUPITER_CONTACT_MARGIN for user configurability.
    int k = 2;

    for(int lbl=1; lbl<=max_label; lbl++){
        BubbleMeta *bm = &bubble_meta[lbl-1];
        bm->bbox[0] = fmax(0, bm->bbox[0] - k);
        bm->bbox[1] = fmin(mx-1, bm->bbox[1] + k);
        bm->bbox[2] = fmax(0, bm->bbox[2] - k);
        bm->bbox[3] = fmin(my-1, bm->bbox[3] + k);
        bm->bbox[4] = fmax(0, bm->bbox[4] - k);
        bm->bbox[5] = fmin(mz-1, bm->bbox[5] + k);
    }

    if(prm->mpi->rank == 0 && prm->cdo->viewflg == 1){
//    if(prm->mpi->rank == 0 && (prm->cdo->viewflg == 1 || 1)){ // debug
        printf("-- Volume information\n");
        for(i=0;i<max_label;i++){
            BubbleMeta *bm = &bubble_meta[i];
            printf("Label %d volume %d\n", bm->label, bm->volume);
        }
    }

    int num_contact_pairs = 0;

#if 1

    // --- Determine contact pairs by two-stage detection ---
    // Stage 1: bbox overlap using margin k (done above). 
    // Stage 2: for each provisional pair, do a stricter local check to reduce false positives.
    int *contact_pairs = NULL;
    detect_bubble_contact(label, bubble_meta, max_label, k,
                        too_small_bubble_volume,
                        nx, ny, nz, mx, my, mz, mxy, m, stm, prm,
                        &contact_pairs, &num_contact_pairs);

#else 

    // --- Determine contact pairs by bounding-box detection ---
    int *contact_pairs = malloc(sizeof(int) * 2 * max_label * max_label);
    for(i=0; i<max_label; i++){
        BubbleMeta *a = &bubble_meta[i];
        for(j=i+1; j<max_label; j++){
            BubbleMeta *b = &bubble_meta[j];
            int overlap =
                (a->bbox[0] <= b->bbox[1] && a->bbox[1] >= b->bbox[0]) &&
                (a->bbox[2] <= b->bbox[3] && a->bbox[3] >= b->bbox[2]) &&
                (a->bbox[4] <= b->bbox[5] && a->bbox[5] >= b->bbox[4]);
            if(overlap){
                contact_pairs[2*num_contact_pairs+0] = a->label;
                contact_pairs[2*num_contact_pairs+1] = b->label;
                num_contact_pairs++;
            }
        }
    }

#endif

    //printf("[Debug] Rank %d num_contact_pairs=%d\n", rank, num_contact_pairs);
    //for(int k=0; k<num_contact_pairs; k++){
    //    printf("[Debug] Contact pair %d: %d - %d\n", k, contact_pairs[2*k], contact_pairs[2*k+1]);
    //}  

    // --- Gather contact pairs to rank 0 ---
    int *recvcounts = NULL, *displs = NULL, *all_pairs = NULL;
    if(rank == 0){
        recvcounts = malloc(sizeof(int)*npe);
        displs = malloc(sizeof(int)*npe);
        all_pairs = malloc(sizeof(int)*2*max_label*max_label*npe);
    }

    int num_elements = num_contact_pairs*2;
    #ifdef JUPITER_MPI
    MPI_Gather(&num_elements, 1, MPI_INT, recvcounts, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if(rank == 0){
        displs[0] = 0;
        for(int r=1; r<npe; r++) displs[r] = displs[r-1] + recvcounts[r-1];
    }
    MPI_Gatherv(contact_pairs, num_elements, MPI_INT,
                all_pairs, recvcounts, displs, MPI_INT, 0, MPI_COMM_WORLD);
    #endif

    // --- Graph coloring (on rank 0) ---
    int *layer_map = malloc(sizeof(int) * max_label);
    int max_layer = 0;

    if(rank==0){
    // --- Check total contact pairs after MPI gather ---
        int total_pairs_all = 0;
        for(int r=0; r<npe; r++){
            //printf("[Debug] recvcounts[%d] = %d\n", r, recvcounts[r]);
            total_pairs_all += recvcounts[r]/2;
        }
        //printf("[Debug] Total contact pairs after gather = %d\n", total_pairs_all);

        int *adj = calloc(max_label*max_label,sizeof(int));
        for(int k=0; k<total_pairs_all; k++){
            int a = all_pairs[2*k]-1;
            int b = all_pairs[2*k+1]-1;

            adj[a*max_label+b] = 1;
            adj[b*max_label+a] = 1;
        }
    // --- (Optional) show adjacency list ---
        //printf("[Debug] Adjacent list\n");
        //for(i=0;i<max_label;i++){
        //    printf("Label %d touches:", i+1);
        //    int count_touch=0;
        //    for(j=0;j<max_label;j++){
        //        if(adj[i*max_label+j]){
        //            printf(" %d", j+1);
        //            count_touch++;
        //        }
        //    }
        //    printf(" (total %d)\n", count_touch);
        //}

    // --- greedy coloring ---
        for(i=0;i<max_label;i++) layer_map[i] = -1;
        max_layer = 0;
        for(i=0;i<max_label;i++){
            BubbleMeta *a = &bubble_meta[i];
            // Small bubbles are usually pushed to the highest layer, but keep
            // inlet-attached bubbles out of that special bucket so surface gas
            // injection can continue to treat them as boundary-connected.
            if(a->volume<too_small_bubble_volume && !a->touches_surface_inlet && !a->has_orifice){
                layer_map[i] = NumberOfLayer-1;
                continue;
            }
            int used[128]={0};
            for(j=0;j<max_label;j++){
                if(adj[i*max_label+j] && layer_map[j]>=0) used[layer_map[j]]=1;
            }
            int c=0; while(used[c]) c++;
            layer_map[i]=c;
            if(c+1>max_layer) max_layer=c+1;

            // --- Debug assigned colors ---
            //printf("[Debug] Label %d assigned layer %d, neighbor layers:", i+1, c);
            //for(j=0;j<max_label;j++){
            //    if(adj[i*max_label+j] && layer_map[j]>=0) printf(" %d", layer_map[j]);
            //}
            //printf("\n");
        }
        free(adj);

    // --- Final layer_map ---
        if(prm->cdo->viewflg == 1){ 
//        if(prm->cdo->viewflg == 1 || 1){  // debug
            printf("-- Final layer_map:\n");
            for(i=0;i<max_label;i++){
                printf("Label %d -> layer %d\n", i+1, layer_map[i]);
            }
            printf("-- Required layers: %d / Maximum layers: %d\n", max_layer, NumberOfLayer);
        }

        if(max_layer > NumberOfLayer){
            printf("[APT Warning] Required number of layers (%d) exceeds NumberOfLayer (%d).\n", max_layer, NumberOfLayer);
            printf("[APT Warning] Increase NumberOfLayer or review bubble counts / partitioning criteria.\n");
            printf("[Debug] Final layer_map:\n");
            for(i=0;i<max_label;i++){
                printf("Label %d -> layer %d\n", i+1, layer_map[i]);
            }
        }
    }

    #ifdef JUPITER_MPI
    MPI_Bcast(layer_map, max_label, MPI_INT, 0, MPI_COMM_WORLD);
    #endif

    zero_clear_int(val->bubble_cnt, NumberOfLayer);
    for(i=0; i<max_label; i++){
        int assigned_layer = layer_map[i];
        if(assigned_layer >= 0 && assigned_layer < NumberOfLayer){
            val->bubble_cnt[assigned_layer]++;
        }
    }

    // --- Determine the layer assignment of cells on each rank ---
    int *new_label = malloc(sizeof(int) * NumberOfLayer * m);
    int *new_is_orifice = malloc(sizeof(int) * NumberOfLayer * m);
    type *new_vof   = malloc(sizeof(type) * NumberOfLayer * m);
    zero_clear_int(new_label, NumberOfLayer*m);
    zero_clear_int(new_is_orifice, NumberOfLayer*m);

    // Initialize VOF as 1 - fs_sum
#pragma omp parallel for private(ilayer,jz,jy,jx,j,jl)
    for(ilayer=0; ilayer<NumberOfLayer; ilayer++){
        for(jz = 0; jz < nz; jz++) {
            for(jy = 0; jy < ny; jy++) {
                for(jx = 0; jx < nx; jx++) {
                    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
                    jl= j+m*ilayer;
                    new_vof[jl]=1.0 - val->fs_sum[j];
                }
            }
        }
    }

#pragma omp parallel for private(ilayer,jz,jy,jx,j,jl)
    for(ilayer=0; ilayer<NumberOfLayer; ilayer++){
        for(jz = label_zs; jz < label_ze; jz++) {
            for(jy = label_ys; jy < label_ye; jy++) {
                for(jx = label_xs; jx < label_xe; jx++) {
                    j = jx + mx*jy + mxy*jz;
                    jl = j + m*ilayer;

                    if(val->is_orifice_layer[jl] != ON)
                        continue;

                    int lbl = label[jl];
                    if(lbl>0 && lbl<=max_label){
                        int new_layer = layer_map[lbl-1];
                        if(new_layer >= NumberOfLayer) {
                            new_layer = NumberOfLayer-1;
                        }
                        new_is_orifice[j+m*new_layer] = ON;
                    }
                }
            }
        }
    }

#pragma omp parallel for private(ilayer,jz,jy,jx,j,jl)
    for(ilayer=0; ilayer<NumberOfLayer; ilayer++){
        for(jz = 0; jz < nz; jz++) {
            for(jy = 0; jy < ny; jy++) {
                for(jx = 0; jx < nx; jx++) {
                    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
                    jl= j+m*ilayer;
                    int lbl = label[jl];
                    if(lbl>0 && lbl<=max_label){
                        int new_layer = layer_map[lbl-1];
                        if(new_layer >= NumberOfLayer) {
                            // !! This is an ad hoc workaround, and ideally this situation should not arise.
                            // printf("[Error] new_layer=%d exceeds NumberOfLayer=%d for label %d. So the bubble with this label is allocated in the highest layer\n",
                            //    new_layer, NumberOfLayer, lbl);
                            new_layer = NumberOfLayer-1;
                        }
                        new_label[j+m*new_layer]=lbl;
                        new_vof[j+m*new_layer]=val->fl_layer[jl];
                    }
                }
            }
        }
    }

#pragma omp parallel for private(ilayer,jz,jy,jx,j,jl)
    for(ilayer=0; ilayer<NumberOfLayer; ilayer++){
        for(jz = 0; jz < nz; jz++) {
            for(jy = 0; jy < ny; jy++) {
                for(jx = 0; jx < nx; jx++) {
                    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
                    jl= j+m*ilayer;
                    val->label_layer[jl]=new_label[jl];
                    val->fl_layer[jl]=new_vof[jl];
                }
            }
        }
    }

    for(ilayer=0; ilayer<NumberOfLayer; ilayer++){
        bcf(&val->fl_layer[ilayer*m],prm);
    }

#pragma omp parallel for
    for(jl=0; jl<NumberOfLayer*m; jl++){
        val->is_orifice_layer[jl] = new_is_orifice[jl];
    }

    bcf_VOF_layer(1,val->fl_layer,val,prm);

    free(bubble_meta);
    free(contact_pairs);
    if(rank==0){
        free(recvcounts); free(displs); free(all_pairs);
    }
    free(layer_map);
    free(new_label); free(new_is_orifice); free(new_vof);

}

void layer_sum_up(variable *val, parameter *prm){

    domain *cdo=prm->cdo;

    int NumberOfLayer = cdo->NumberOfLayer;

    int nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
        mx=cdo->mx, my=cdo->my, mz=cdo->mz, 
        mxy=cdo->mxy, m=cdo->m, stm=cdo->stm;

    int ilayer,jz,jy,jx,j,jl;

#pragma omp parallel for private(ilayer,jz,jy,jx,j,jl)
    for(jz = 0; jz < nz; jz++) {
        for(jy = 0; jy < ny; jy++) {
            for(jx = 0; jx < nx; jx++) {
                j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

                type vall_c = 0;

                for(ilayer = 0; ilayer<NumberOfLayer; ilayer++){
                    jl= j+m*ilayer;
                    vall_c += 1-val->fl_layer[jl];
                }
                
                // Under the multi-layer model, only fl_0 (material ID 0) holds values,
                // so val->fl[j] alone is sufficient (no NBaseComponent offset needed).
                val->fl[j] = 1-clip(vall_c);

            }
        }
    }

    bcf(val->fl, prm);

}

void search_liquid_film(variable *val, parameter *prm){

    domain *cdo=prm->cdo;

    int NumberOfLayer = cdo->NumberOfLayer;

    int nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
        mx=cdo->mx, my=cdo->my, mz=cdo->mz, 
        mxy=cdo->mxy, m=cdo->m, stm=cdo->stm;

    int ilayer,jz,jy,jx,j,jl;

    type threshold = 0.5;

    type neighbor_distance = cdo->film_cell * cdo->dx; // tentative

#pragma omp parallel for private(ilayer,jz,jy,jx,j,jl)
    for(jz = 0; jz < nz; jz++) {
        for(jy = 0; jy < ny; jy++) {
            for(jx = 0; jx < nx; jx++) {
                j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

                if(val->fls[j]<threshold){
                    val->liquid_film[j] = 0.0;
                    continue; //  if half or more of the cell is filled with gas, skip
                } 

                int neighbor_objects_count = 0; // object may be bubble, IBM structure or solid boundary
                type film_thickness = 0.0;

                         // NOTE: ls is negative outside solid regions
                if(val->ls[j] < 0 && fabs(val->ls[j]) < neighbor_distance){
                    neighbor_objects_count += 1;
                    film_thickness += fabs(val->ls[j]);
                }

                                               // vvv the highest layer is supposed to contain only small fragments, which is out of liquid film search scope
                for(ilayer=0; ilayer<NumberOfLayer - 1; ilayer++){
                    jl = j + ilayer*m;
                                     // NOTE: lls is positive in liquid regions
                    if(val->lls_layer[jl] > 0 && fabs(val->lls_layer[jl]) < neighbor_distance){ 
                        neighbor_objects_count += 1;
                        film_thickness += fabs(val->lls_layer[jl]);
                    }
                }

                if(neighbor_objects_count < 2){
                    val->liquid_film[j] = 0.0;
                }else if(neighbor_objects_count == 2){
                    if(film_thickness < neighbor_distance) val->liquid_film[j] = film_thickness;
                    else val->liquid_film[j] = 0.0;
                }else{
                    // the theory is not generalized to the situation where neighboring objects are more than 3
                    // hence the below is tentative
                    if(film_thickness * (2/neighbor_objects_count) < neighbor_distance) val->liquid_film[j] = film_thickness * (2/neighbor_objects_count);
                    else val->liquid_film[j] = 0.0;
                }

            }
        }
    }

    bcf(val->liquid_film, prm);

}


type multi_layer(variable *val, parameter *prm, int flag){
    
    // flag = 1: during simulation, need to sum fl_layer to update fl_0.
    // flag = 0: at simulation start fl_0 is provided by user, so no summation is needed.
    
    type time0=cpu_time();
    if(prm->flg->multi_layer==OFF) return cpu_time() - time0;

    domain *cdo=prm->cdo;

    int NumberOfLayer = cdo->NumberOfLayer;

    int nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
        mx=cdo->mx, my=cdo->my, mz=cdo->mz, 
        mxy=cdo->mxy, m=cdo->m, stcl=cdo->stm;

    int j, ilayer;

    int max_label=0;
    zero_clear_int(val->bubble_cnt, cdo->NumberOfLayer);

    if(prm->flg->multi_layer_less_coalescence==ON){
        
    // When using the less_coalescence model, vof_advection (performed before
    // multi_layer) advects fl, not fl_layer. Therefore the latest VOF is stored in fl, and this
    // VOF should be distributed to all fl_layer entries.

        define_fl_layer_from_fl(val,prm);

    }

    if(!val->is_orifice_layer_initialized){
        initialize_orifice_layer_state(val, prm);
    }

    // Connected Component Labeling (VOF < 0.5) on each layer
    for(ilayer=0; ilayer<NumberOfLayer; ilayer++){
        val->bubble_cnt[ilayer] = CCL(val, prm, ilayer, max_label); 
        max_label += val->bubble_cnt[ilayer];
    } 

    if(max_label==0) return cpu_time() - time0;

    // Define active cells as phase-labeled cells plus peripheral cells
    // (peripheral = +1 cells outward). Use bounding-box overlap of active
    // cells to detect contacts. For contacting bubble pairs, raise one
    // bubble's VOF and label to the upper layer.
    APT(val, prm, max_label); 

    if(flag && prm->flg->multi_layer_no_coalescence) layer_sum_up(val, prm);    

    return cpu_time() - time0;

}
