import sys
import numpy as np

#********************************************************************
# number of cell
NX =  50
NY =  50
NZ =  100
# domain
XD = 500.0e-3
YD = 500.0e-3
ZD = 1000.0e-3
# Origin of coordinates
XO = 0.0
YO = 0.0
ZO = 0.0
# time interval
DT = 0.01
# Number of Component
#NCOMP = 2
#precision
PRECISION = 4
#PRECISION = 8
# name
NAME = []
NAME.append("VOF_l_0 [-]")
NAME.append("VOF_l_1 [-]")
NAME.append("ll [-]")
NAME.append("solid_0 [-]")
NAME.append("solid_1 [-]")
NAME.append("div_u")
#NAME.append("pressure")
#NAME.append("density[kg/m3]")
NAME.append("velocity vector [m/s]")
# data name
DNAME = []
DNAME.append("./vdata/fl_0/fl_0_")
DNAME.append("./vdata/fl_1/fl_1_")
DNAME.append("./vdata/lls/lls_")
DNAME.append("./vdata/fs_0/fs_0_")
DNAME.append("./vdata/fs_1/fs_1_")
DNAME.append("./vdata/div_u/div_u_")
#DNAME.append("./vdata/p/p_")
#DNAME.append("./vdata/dens/dens_")
DNAME.append("./vdata/uvw/uvw_")
# scalar or vector
SV = ["s","s","s", "s","s","s","v"]
#SV = ["s","s","s","s","v"]
# corrdinate 0:OFF, 1:ON
ICO = 1
#********************************************************************

def output_coordinate(dx,dy,dz):
  ijkmax = NX*NY*NZ
  xx = np.zeros(ijkmax,dtype=np.float32)
  yy = np.zeros(ijkmax,dtype=np.float32)
  zz = np.zeros(ijkmax,dtype=np.float32)
  for k in range(NZ):
    znow = k*dz
    for j in range(NY):
      ynow = j*dy
      for i in range(NX):
        xnow = i*dx
        ijk = i + j*NX + k*NX*NY
        xx[ijk] = xnow
        yy[ijk] = ynow
        zz[ijk] = znow
  with open("xcoo","wb") as f1:
    xx.tofile("xcoo")
  with open("ycoo","wb") as f2:
    yy.tofile("ycoo")
  with open("zcoo","wb") as f3:
    zz.tofile("zcoo")

def write_scalar (name,dname):
  s  = "    <Attribute Name=\"%s\" AttributeType=\"Scalar\" Center=\"Cell\">\n" % name
  s += "      <DataItem Format=\"Binary\" Dimensions=\"%d %d %d\" NumberType=\"Float\" Precision=\"%d\" Seek=\"0\" Endian=\"Little\">\n" % (NZ, NY, NX, PRECISION)
  s += "        %s\n" % dname
  s += "      </DataItem>\n"
  s += "    </Attribute>\n"
  return s

def write_coordinate():
  s  = "    <Attribute Name=\"x\" AttributeType=\"Scalar\" Center=\"Cell\">\n"
  s += "      <DataItem Format=\"Binary\" Dimensions=\"%d %d %d\" NumberType=\"Float\" Precision=\"%d\" Seek=\"0\" Endian=\"Little\">\n" % (NZ, NY, NX, 4)
  s += "        xcoo\n"
  s += "      </DataItem>\n"
  s += "    </Attribute>\n"

  s += "    <Attribute Name=\"y\" AttributeType=\"Scalar\" Center=\"Cell\">\n"
  s += "      <DataItem Format=\"Binary\" Dimensions=\"%d %d %d\" NumberType=\"Float\" Precision=\"%d\" Seek=\"0\" Endian=\"Little\">\n" % (NZ, NY, NX, 4)
  s += "        ycoo\n"
  s += "      </DataItem>\n"
  s += "    </Attribute>\n"

  s += "    <Attribute Name=\"z\" AttributeType=\"Scalar\" Center=\"Cell\">\n"
  s += "      <DataItem Format=\"Binary\" Dimensions=\"%d %d %d\" NumberType=\"Float\" Precision=\"%d\" Seek=\"0\" Endian=\"Little\">\n" % (NZ, NY, NX, 4)
  s += "        zcoo\n"
  s += "      </DataItem>\n"
  s += "    </Attribute>\n"
  return s

def write_vector (name,dname):
  s  = "    <Attribute Name=\"%s\" Active=\"1\" AttributeType=\"Vector\" Center=\"Cell\">\n" % name
  s += "      <DataItem Format=\"Binary\" Dimensions=\"%d %d %d 3\" NumberType=\"Float\" Precision=\"%d\" Endian=\"Little\">\n" % (NZ, NY, NX, PRECISION)
  s += "        %s\n" % dname
  s += "      </DataItem>\n"
  s += "    </Attribute>\n"
  return s

def XdmfGrid(iout,time,dx,dy,dz):
  s  = "  <Grid Name=\"Something\">\n"
  s += "    <Topology TopologyType=\"3DRectMesh\" Dimensions=\"%d %d %d\"/>\n" % (NZ+1, NY+1, NX+1)
  s += "    <Geometry GeometryType=\"Origin_DxDyDz\">\n"
  s += "      <DataItem Name=\"Origin\" Format=\"XML\" Dimensions=\"3\"> %f %f %f </DataItem>\n" % (XO, YO, ZO)
  s += "      <DataItem Name=\"DxDyDz\" Format=\"XML\" Dimensions=\"3\"> %f %f %f </DataItem>\n" % (dx, dy, dz)
  s += "    </Geometry>\n"

  ioutstr = str(iout)
  ioutstr_z = ioutstr.zfill(4)
  dataname = []
  for i in range(len(DNAME)):
    dataname.append(DNAME[i] + ioutstr_z + ".dat")

  for i in range(len(NAME)):
    if SV[i] == "s":
      s += write_scalar(NAME[i],dataname[i])
    elif SV[i] == "v":
      s += write_vector(NAME[i],dataname[i])

  if ICO == 1:
    s += write_coordinate()
  # Time
  s += "    <Time Value=\"%f\"/>\n" % time
  s += "  </Grid>\n"

  return s

def XdmfString(istart,iend,iwidth,dx,dy,dz):
  
  s  = "<?xml version=\"1.0\"?>\n"
  s += "<Xdmf Version=\"2.0\">\n"
  s += "<Domain Name=\"Something\">\n"
  s += "<Grid Name=\"TimeSeries\" CollectionType=\"Temporal\" GridType=\"Collection\">\n"

  s += "  <Time TimeType=\"HyperSlab\">\n"
  s += "    <DataItem Format=\"XML\" Dimensions=\"3\"> %f %f %f </DataItem>\n" % (istart*DT, DT*iwidth, int((iend-istart)/iwidth)+1)
  s += "  </Time>\n"

  for i in range(istart, iend+1,iwidth):
    time = i*DT
    s += XdmfGrid(i, time, dx,dy,dz)

  s += "</Grid>\n"
  s += "</Domain>\n"
  s += "</Xdmf>\n"
  return s

def main():

  # mesh size
  dx = XD/NX
  dy = YD/NY
  dz = ZD/NZ

  istart = int(sys.argv[1])
  iend   = int(sys.argv[2])
  iwidth = int(sys.argv[3])

  # output corrdinate
  if ICO == 1:
    output_coordinate(dx,dy,dz)

  s = XdmfString(istart, iend, iwidth,dx,dy,dz)
  # output file name
 #outfilename = "XDMF_data_series_%04d_%04d.xmf" % (istart, iend)
  outfilename = "XDMF_data_series.xmf"
  fout = open(outfilename, 'w')
  fout.write(s)
  fout.close()
  print( "Write %s" % outfilename)

if __name__ == "__main__":
  main()
