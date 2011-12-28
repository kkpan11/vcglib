/****************************************************************************
* VCGLib                                                            o o     *
* Visual and Computer Graphics Library                            o     o   *
*                                                                _   O  _   *
* Copyright(C) 2004                                                \/)\/    *
* Visual Computing Lab                                            /\/|      *
* ISTI - Italian National Research Council                           |      *
*                                                                    \      *
* All rights reserved.                                                      *
*                                                                           *
* This program is free software; you can redistribute it and/or modify      *   
* it under the terms of the GNU General Public License as published by      *
* the Free Software Foundation; either version 2 of the License, or         *
* (at your option) any later version.                                       *
*                                                                           *
* This program is distributed in the hope that it will be useful,           *
* but WITHOUT ANY WARRANTY; without even the implied warranty of            *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
* GNU General Public License (http://www.gnu.org/licenses/gpl.txt)          *
* for more details.                                                         *
*                                                                           *
****************************************************************************/
#ifndef __VCGLIB_IMPORTERBUNDLER
#define __VCGLIB_IMPORTERBUNDLER

#include <stddef.h>
#include <stdio.h>
#include <wrap/callback.h>
#include <wrap/io_trimesh/io_mask.h>
#include <wrap/Exif/include/Exif/jhead.h>


namespace vcg {
namespace tri {
namespace io {

	struct Correspondence{
		Correspondence(unsigned int id_img_,unsigned int key_,float x_,float y_):id_img(id_img_),key(key_),x(x_),y(y_){};
		unsigned int id_img,key;
		float x;
		float y;
	};

	typedef std::vector<Correspondence> CorrVec;

/** 
This class encapsulate a filter for opening bundler file
*/
template <class OpenMeshType>
class ImporterOUT
{
public:

typedef typename OpenMeshType::VertexPointer VertexPointer;
typedef typename OpenMeshType::ScalarType ScalarType;
typedef typename OpenMeshType::VertexType VertexType;
typedef typename OpenMeshType::FaceType FaceType;
typedef typename OpenMeshType::VertexIterator VertexIterator;
typedef typename OpenMeshType::FaceIterator FaceIterator;
typedef typename OpenMeshType::EdgeIterator EdgeIterator;

static char *readline(FILE *fp, int max=100){
    int i=0;
    char c, *str = new char[max];
    fscanf(fp, "%c", &c);
    while( (c!=10) && (c!=13) && (i<max) ){
        str[i++] = c;
        fscanf(fp, "%c", &c);
    }
    str[i] = '\0'; //end of string
    return str;
}

;

static bool ReadHeader(FILE *fp,unsigned int &num_cams, unsigned int &num_points){
    char *line;
    line = readline(fp); if( (!line) || (0!=strcmp("# Bundle file v0.3", line)) ) return false;
    line = readline(fp); if(!line) return false;
    sscanf(line, "%d %d", &num_cams, &num_points);
	return true;
}

static bool ReadHeader(const char * filename,unsigned int &num_cams, unsigned int &num_points){
	FILE *fp = fopen(s.c_str(), "r");
	if(!fp) return false;
	ReadHeader(fp);
	fclose(fp);
	return true;
}


static int Open( OpenMeshType &m, std::vector<Shot<ScalarType> >  & shots, 
							std::vector<std::string > & image_filenames, 
				const char * filename,const char * filename_images, CallBackPos *cb=0)
{
	unsigned int   num_cams,num_points;
 
	FILE *fp = fopen(filename,"r");
	if(!fp) return false;
	ReadHeader(fp, num_cams,  num_points);
	char *line;

	ReadImagesFilenames(filename_images,image_filenames);

	shots.resize(num_cams);
	for(int i = 0; i < num_cams;++i)
	{
        float f, k1, k2;
		float R[16]={0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1};
        vcg::Point3f t;
 
        line = readline(fp); if(!line) return false; sscanf(line, "%f %f %f", &f, &k1, &k2);

        line = readline(fp); if(!line) return false; sscanf(line, "%f %f %f", &(R[0]), &(R[1]), &(R[2]));  R[3] = 0;
        line = readline(fp); if(!line) return false; sscanf(line, "%f %f %f", &(R[4]), &(R[5]), &(R[6]));  R[7] = 0;
        line = readline(fp); if(!line) return false; sscanf(line, "%f %f %f", &(R[8]), &(R[9]), &(R[10])); R[11] = 0;

        line = readline(fp); if(!line) return false; sscanf(line, "%f %f %f", &(t[0]), &(t[1]), &(t[2]));

        vcg::Matrix44f mat = vcg::Matrix44<vcg::Shotf::ScalarType>::Construct<float>(R);

        vcg::Matrix33f Rt = vcg::Matrix33f( vcg::Matrix44f(mat), 3);
        Rt.Transpose();

        vcg::Point3f pos = Rt * vcg::Point3f(t[0], t[1], t[2]);

        shots[i].Extrinsics.SetTra(vcg::Point3<vcg::Shotf::ScalarType>::Construct<float>(-pos[0],-pos[1],-pos[2]));
        shots[i].Extrinsics.SetRot(mat);

        shots[i].Intrinsics.FocalMm    = f;
		shot.Intrinsics.k[0] = 0.0;//k1; To be uncommented when distortion is taken into account reliably
		shot.Intrinsics.k[1] = 0.0;//k2;
		AddIntrinsics(shots[i],image_filenames[i].c_str());
    }

	// load all correspondences
	OpenMeshType::template PerVertexAttributeHandle<CorrVec> ch = vcg::tri::Allocator<OpenMeshType>::GetPerVertexAttribute<CorrVec>(m,"correspondences");
	if(!vcg::tri::Allocator<OpenMeshType>::IsValidHandle(m,ch))
		ch = vcg::tri::Allocator<OpenMeshType>::AddPerVertexAttribute<CorrVec>(m,"correspondences");

	OpenMeshType::VertexIterator vi = vcg::tri::Allocator<OpenMeshType>::AddVertices(m,num_points);
	for(int i = 0; i < num_points;++i,++vi){
		float x,y,z; 
		unsigned int r,g,b,i_cam, key_sift,n_corr;
		fscanf(fp,"%f %f %f ",&x,&y,&z);
		(*vi).P() = vcg::Point3<OpenMeshType::ScalarType>(x,y,z);
		fscanf(fp,"%d %d %d ",&r,&g,&b);
		(*vi).C() = vcg::Color4b(r,g,b,255);

		fscanf(fp,"%d ",&n_corr);
		for(int j = 0; j < n_corr; ++j){
			fscanf(fp,"%d %d %f %f ",&i_cam,&key_sift,&x,&y);
			Correspondence corr(i_cam,key_sift,x,y);
			ch[i].push_back(corr);
		}
	}
	vcg::tri::UpdateBounding<OpenMeshType>::Box(m);
    fclose(fp);

    return (shots.size() == 0);
}

static int Open( OpenMeshType &m, std::vector<Shot<ScalarType> >  shots, const char * filename_out,const char * filename_list, CallBackPos *cb=0){
	QStringList image_files;
	ReadHeader(filename_out);
	ReadImagesFilenames(QString(filename_list),list,num_cams);
	return Open( m, shots,filename, image_files,  cb);

}

static bool ReadImagesFilenames(const char *  filename,std::vector<std::string> &image_filenames)
{
	char  line[1000],name[1000];

	FILE * fi = fopen(filename,"r");
	if (!fi) return false;
    else
    { 
		while(!feof(fi)){
			fgets (line , 1000, fi);
			sscanf(line,"%s",&name[0]); 
			std::string n(name);
			image_filenames.push_back(n);
		}
	}
	fclose(fi);
    return true;
}

static bool  AddIntrinsics(vcg::Shotf &shot, const char * image_file)
{
  ::ProcessFile(image_file);
  shot.Intrinsics.ViewportPx = vcg::Point2i(ImageInfo.Width, ImageInfo.Height);
  shot.Intrinsics.CenterPx   = vcg::Point2f(float(ImageInfo.Width/2.0), float(ImageInfo.Height/2.0));

  shot.Intrinsics.PixelSizeMm = vcg::Point2f(1,1);
  return true;
}
}; // end class



} // end namespace tri
} // end namespace io
} // end namespace vcg

#endif
