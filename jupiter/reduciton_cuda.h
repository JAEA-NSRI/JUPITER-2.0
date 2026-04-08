
#ifndef HEADER_REDUCTIOCUDA
#define HEADER_REDUCTIOCUDA

// __device__ static type warpReduceSum(type val);
// __device__ static void local_sum_cuda(type val,type *val_ret,int thid);

__device__ static type warpReduceSum(type val){
  for(int offset = warpSize/2;offset >0; offset/=2){
    //    val+= __shfl_down_sync(0xffffffff,val,offset);
    val = val + __shfl_down_sync(0xffffffff,val,offset);
  }
  return val;
}

__device__ static void local_sum_cuda(type val,type *val_ret,int thid){
  type warp_val = warpReduceSum(val);
  int lane = thid % warpSize;
  if(lane==0)atomicAdd(&val_ret[0], warp_val);
  return ;
}

#endif
