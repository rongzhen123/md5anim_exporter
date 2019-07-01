//**************************************************************************/
// Copyright (c) 1998-2007 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/
// DESCRIPTION: Appwizard generated plugin
// AUTHOR: 
//***************************************************************************/

#include "md5animation_exporter.h"
#include "../common/SimpleFile.h"
#include "../common/MyErrorProc.h"
#include "../common/Define.h"
#include <IGame/IGame.h>
#include <IGame/IConversionManager.h>
#include <IGame/IGameModifier.h>
#include <conio.h>
#include <iostream>
#include <hash_map>
#include <map>
#include <algorithm>
#include <atlconv.h> 

using namespace std;


/************************************************************************/
/* 

MD5Version <int:version>
commandline <string:commandline>

numFrames <int:numFrames>
numJoints <int:numJoints>
frameRate <int:frameRate>
numAnimatedComponents <int:numAnimatedComponents>

hierarchy {
<string:jointName> <int:parentIndex> <int:flags> <int:startIndex>
...
}

bounds {
( vec3:boundMin ) ( vec3:boundMax )
...
}

baseframe {
( vec3:position ) ( vec3:orientation )
...
}

frame <int:frameNum> {
<float:frameData> ...
}
...

*/
/*************************************************************************/

#define md5animation_exporter_CLASS_ID	Class_ID(0x678e12ea, 0x70095f88)



struct BoneInfo 
{
	BoneInfo()
		:Name(NULL),
		SelfIndex(-1),
		ParentIndex(-1),
		Node(NULL),
		Flag(0)
	{

	}
	MCHAR* Name;
	int SelfIndex;
	int ParentIndex;
	IGameNode* Node;
	int Flag;
	Point3 Pos;
	Quat Rot;

	bool operator < (const BoneInfo &b) const
	{
		return MaxAlphaNumComp(Name,b.Name)<0;
	}
};



//ȥ���ַ����ұ߿ո�
void Str_RTrim(WCHAR* pStr)
{
	WCHAR* pTmp = pStr + wcslen(pStr) - 1;
	while(*pTmp == L' ')
	{
		*pTmp = L'\0';
		pTmp--;
	}
}
//ȥ���ַ�����߿ո�
void StrLTrim(WCHAR* pStr)
{
	WCHAR* pTmp = pStr;
	while(*pTmp == L' ')
	{
		pTmp++;
	}
	while(*pTmp !=L'\0')
	{
		*pStr = *pTmp;
		pStr++;
		pTmp++;
	}
	*pStr=L'\0';
}
void Str_trim(WCHAR* pStr)
{
	WCHAR* pTmp = pStr;
	while(*pStr != L'\0')
	{
		if(*pStr != L' ')
		{
			*pTmp++ = *pStr;
		}
		++pStr;
	}
	*pTmp = L'\0';
}
//���ַ����еĿո����»��ߴ���
void Str_replace_place_with_underline(WCHAR* pStr)
{
	StrLTrim(pStr);
	Str_RTrim(pStr);
	WCHAR* pTmp = pStr;
	while(*pStr != L'\0')
	{
		if(*pStr != L' ')
		{
			*pTmp++ = *pStr;
		}
		else
		{
			*pTmp++ = L'_';
		}
		++pStr;
	}
	*pTmp = L'\0';
}


//��ͷ���򰴲㼶 �Թ������ ������˳��
void BoneSort(vector<BoneInfo>& boneList)
{
	vector<BoneInfo> tmpList(boneList.begin(),boneList.end());
	boneList.clear();
	vector<BoneInfo>::iterator it=tmpList.begin();

	boneList.push_back(*it);
	tmpList.erase(it);

	int parentIndex=0;
	while (!tmpList.empty())
	{
		BoneInfo& parent=boneList.at(parentIndex);
		vector<BoneInfo>::iterator childIt=tmpList.begin();
		vector<BoneInfo> children;
		while (childIt!=tmpList.end())
		{
			if (childIt->ParentIndex==parent.SelfIndex)
			{
				children.push_back(*childIt);
				tmpList.erase(childIt);
				childIt=tmpList.begin();
			}
			else
				++childIt;
		}
		sort(children.begin(),children.end());
		boneList.insert(boneList.end(),children.begin(),children.end());
		++parentIndex;
	}
}


class md5animation_exporter : public SceneExport {
public:
	//Constructor/Destructor
	md5animation_exporter();
	~md5animation_exporter();

	static HWND hParams;

	int				ExtCount();					// Number of extensions supported
	const TCHAR *	Ext(int n);					// Extension #n (i.e. "3DS")
	const TCHAR *	LongDesc();					// Long ASCII description (i.e. "Autodesk 3D Studio File")
	const TCHAR *	ShortDesc();				// Short ASCII description (i.e. "3D Studio")
	const TCHAR *	AuthorName();				// ASCII Author name
	const TCHAR *	CopyrightMessage();			// ASCII Copyright message
	const TCHAR *	OtherMessage1();			// Other message #1
	const TCHAR *	OtherMessage2();			// Other message #2
	unsigned int	Version();					// Version number * 100 (i.e. v3.01 = 301)
	void			ShowAbout(HWND hWnd);		// Show DLL's "About..." box

	BOOL SupportsOptions(int ext, DWORD options);
	int  DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts=FALSE, DWORD options=0);

public:
	TimeValue _TvToDump;
	int _FrameCount;
	int _BoneCount;
	int _FrameRate;
	int _AnimatedCount;

	BOOL _IncludeBounds;
	BOOL _HelperObject;
	BOOL _DoomVersion;

	FILE* _OutFile;

	IGameScene * pIgame;

	vector<BoneInfo> _BoneList;

	int SaveMd5Anim( ExpInterface * ei, Interface * gi ) 
	{
		if (!_DoomVersion)
			fprintf(_OutFile,"MD5Version 4843\r\ncommandline \"by HoneyCat md5animExporter v%d\"\r\n\r\n",Version());
		else
			fprintf(_OutFile,"MD5Version 10\r\ncommandline \"by HoneyCat md5animExporter v%d\"\r\n\r\n",Version());

		DumpCount(gi);

		DumpHierarchy();

		if (_IncludeBounds)
			DumpBounds();

		DumpBaseFrame();

		DumpFrames();

		fflush(_OutFile);
		return TRUE;
	}

	void DumpCount(Interface * gi) 
	{
		Interval animation=gi->GetAnimRange();
		_FrameCount=(animation.End()-animation.Start())/GetTicksPerFrame()+1;
		_BoneCount=0;
		_FrameRate=GetFrameRate();
		_AnimatedCount=0;

		BoneInfo bone;
		bone.Name=L"origin";
		bone.SelfIndex=(int)_BoneList.size();
		bone.ParentIndex=-1;
		bone.Node=NULL;
		bone.Flag=0;
		bone.Pos=Point3::Origin;
		bone.Rot=Quat();

		_BoneList.push_back(bone);
		_BoneCount++;

		for(int loop = 0; loop <pIgame->GetTopLevelNodeCount();loop++)
		{
			IGameNode * pGameNode = pIgame->GetTopLevelNode(loop);
			CountNodes(pGameNode,loop,0);
		}

		fprintf(_OutFile,"numFrames %d\r\n",_FrameCount);
		fprintf(_OutFile,"numJoints %d\r\n",_BoneCount);
		fprintf(_OutFile,"frameRate %d\r\n",_FrameRate);
		fprintf(_OutFile,"numAnimatedComponents %d\r\n\r\n",_AnimatedCount);
	}

	void CountNodes( IGameNode * pGameNode ,int NodeIndex,int ParentIndex=-1) 
	{
		int index=(int)_BoneList.size();
		IGameObject * obj = pGameNode->GetIGameObject();
		if ((obj->GetIGameType()==IGameObject::IGAME_BONE&&
			(_HelperObject||obj->GetMaxObject()->SuperClassID()!=HELPER_CLASS_ID))||
			(_HelperObject&&obj->GetIGameType()==IGameObject::IGAME_HELPER))
		{
			_BoneCount++;

			BoneInfo bone;
			USES_CONVERSION;
			bone.Name=(WCHAR*)pGameNode->GetName();
			bone.SelfIndex=(int)_BoneList.size();
			bone.ParentIndex=ParentIndex;
			bone.Node=pGameNode;

			BasePose(pGameNode,obj,bone);

			CountAnimated(pGameNode,obj,bone);

			_BoneList.push_back(bone);
		}

		//fprintf(_OutFile,"%s %d\n",pGameNode->GetName(),obj->GetIGameType());
		pGameNode->ReleaseIGameObject();

		for(int i=0;i<pGameNode->GetChildCount();i++)
		{
			IGameNode * child = pGameNode->GetNodeChild(i);

			CountNodes(child,i,index);
		}
	}

	void BasePose( IGameNode * pGameNode,IGameObject * obj , BoneInfo& bone ) 
	{
		//GetLocalTM��ֵʵ������max�ڵ������任��һ����
		GMatrix mat;
		IGameNode * parentNode=pGameNode->GetNodeParent();
		if (parentNode)
		{
			mat=parentNode->GetWorldTM(_TvToDump);
			mat=ToRightHand(mat);
			mat=mat.Inverse();
		}
		mat=pGameNode->GetWorldTM(_TvToDump)*mat;
		mat=ToRightHand(mat);

		bone.Pos=mat.Translation();
		bone.Rot=mat.Rotation();
	}

	GMatrix ToRightHand(const GMatrix& mat) const
	{
		Point3 pos=mat.Translation();
		Quat quat=mat.Rotation();
		Matrix3 ret=mat.ExtractMatrix3();
		//����Ƿ��Ǿ���
		bool isMirrored=DotProd( CrossProd( ret.GetRow(0).Normalize(), ret.GetRow(1).Normalize() ).Normalize(), ret.GetRow(2).Normalize() ) < 0;
		//�������Ծ��������ת������
		//������ʽ���д�����
		if (isMirrored)
		{
			float tmp;
			tmp=quat.x;
			quat.x=-quat.y;
			quat.y=tmp;
			tmp=quat.z;
			quat.z=quat.w;
			quat.w=-tmp;
		}
		ret.SetRotate(quat);
		ret.SetTrans(pos);
		return GMatrix(ret);
	}

	bool AlmostEqual2sComplement(float A, float B, int maxUlps=3)
	{
		// Make sure maxUlps is non-negative and small enough that the
		// default NAN won't compare as equal to anything.
		assert(maxUlps > 0 && maxUlps < 4 * 1024 * 1024);
		int aInt = *(int*)&A;
		// Make aInt lexicographically ordered as a twos-complement int
		if (aInt < 0)
			aInt = 0x80000000 - aInt;
		// Make bInt lexicographically ordered as a twos-complement int
		int bInt = *(int*)&B;
		if (bInt < 0)
			bInt = 0x80000000 - bInt;
		int intDiff = abs(aInt - bInt);
		if (intDiff <= maxUlps)
			return true;
		return false;
	}

	void CountAnimated(IGameNode * pGameNode,IGameObject * obj,BoneInfo & bone) 
	{
		int flag=0;

		TimeValue start=pIgame->GetSceneStartTime();
		TimeValue end=pIgame->GetSceneEndTime();
		TimeValue ticks=pIgame->GetSceneTicks();

		for (TimeValue tv = start; tv <= end; tv += ticks)
		{
			GMatrix mat;
			IGameNode * parentNode=pGameNode->GetNodeParent();
			if (parentNode)
			{
				mat=parentNode->GetWorldTM(tv);
				mat=ToRightHand(mat);
				mat=mat.Inverse();
			}
			mat=pGameNode->GetWorldTM(tv)*mat;
			mat=ToRightHand(mat);
			Point3 pos=mat.Translation();
			Quat rot=mat.Rotation();

			if (!AlmostEqual2sComplement(pos.x,bone.Pos.x))
			{
				flag|=eHierarchyFlag_Pos_X;
			}
			if (!AlmostEqual2sComplement(pos.y,bone.Pos.y))
			{
				flag|=eHierarchyFlag_Pos_Y;
			}
			if (!AlmostEqual2sComplement(pos.z,bone.Pos.z))
			{
				flag|=eHierarchyFlag_Pos_Z;
			}

			if (!AlmostEqual2sComplement(rot.x,bone.Rot.x))
			{
				flag|=eHierarchyFlag_Rot_X;
			}
			if (!AlmostEqual2sComplement(rot.y,bone.Rot.y))
			{
				flag|=eHierarchyFlag_Rot_Y;
			}
			if (!AlmostEqual2sComplement(rot.z,bone.Rot.z))
			{
				flag|=eHierarchyFlag_Rot_Z;
			}
		}

		if (flag&eHierarchyFlag_Pos_X)
			_AnimatedCount++;
		if (flag&eHierarchyFlag_Pos_Y)
			_AnimatedCount++;
		if (flag&eHierarchyFlag_Pos_Z)
			_AnimatedCount++;

		if (flag&eHierarchyFlag_Rot_X)
			_AnimatedCount++;
		if (flag&eHierarchyFlag_Rot_Y)
			_AnimatedCount++;
		if (flag&eHierarchyFlag_Rot_Z)
			_AnimatedCount++;

		bone.Flag=flag;
	}

	void DumpHierarchy() 
	{
		//Ϊ�˱�֤�����������˳��һ��������
		BoneSort(_BoneList);

		for (int b=0;b<_BoneCount;++b)
		{
			BoneInfo& info=_BoneList.at(b);
			if (info.ParentIndex>-1)
			{
				for (int p=0;p<_BoneList.size();++p)
				{
					BoneInfo& parentInfo=_BoneList.at(p);
					if (parentInfo.SelfIndex==info.ParentIndex)
					{
						info.ParentIndex=p;
						break;
					}
				}
			}
		}

		fprintf(_OutFile,"hierarchy {\r\n");
		int startIndex=0;
		for(int i = 0; i <_BoneCount;i++)
		{
			BoneInfo& info=_BoneList.at(i);
			info.SelfIndex=i;
			WCHAR tmp[100];
			wcscpy(tmp,info.Name);
			Str_replace_place_with_underline(tmp);
			USES_CONVERSION;
			fprintf(_OutFile,"\t\"%s\" %d %d %d\r\n",W2A(tmp),info.ParentIndex,info.Flag,startIndex);

			if (info.Flag&eHierarchyFlag_Pos_X)
				startIndex++;
			if (info.Flag&eHierarchyFlag_Pos_Y)
				startIndex++;
			if (info.Flag&eHierarchyFlag_Pos_Z)
				startIndex++;

			if (info.Flag&eHierarchyFlag_Rot_X)
				startIndex++;
			if (info.Flag&eHierarchyFlag_Rot_Y)
				startIndex++;
			if (info.Flag&eHierarchyFlag_Rot_Z)
				startIndex++;
		}
		fprintf(_OutFile,"}\r\n\r\n");
	}

	void DumpBounds() 
	{
		fprintf(_OutFile,"bounds {\r\n");

		TimeValue start=pIgame->GetSceneStartTime();
		TimeValue end=pIgame->GetSceneEndTime();
		TimeValue ticks=pIgame->GetSceneTicks();

		for (TimeValue tv = start; tv <= end; tv += ticks)
		{
			Box3 objBound;
			for(int loop = 0; loop <pIgame->GetTopLevelNodeCount();loop++)
			{
				IGameNode * pGameNode = pIgame->GetTopLevelNode(loop);
				IGameObject * obj = pGameNode->GetIGameObject();
				if (!pGameNode->IsNodeHidden()&&
					obj->GetIGameType()==IGameObject::IGAME_MESH)
				{
					INode* maxNode=pGameNode->GetMaxNode();
					const ObjectState& objState=maxNode->EvalWorldState(tv);
					Object* maxObj=objState.obj;
					Box3 bb;
					Matrix3 mat=maxNode->GetNodeTM(tv);
					maxObj->GetDeformBBox(tv,bb,&mat);
					objBound+=bb;
				}
			}

			Point3 min=objBound.Min()-_BoneList.at(0).Pos;
			Point3 max=objBound.Max()-_BoneList.at(0).Pos;
			fprintf(_OutFile,"\t( %f %f %f ) ( %f %f %f )\r\n",
				min.x,min.y,min.z,
				max.x,max.y,max.z);
		}


		fprintf(_OutFile,"}\r\n\r\n");
	}

	void DumpBaseFrame() 
	{
		fprintf(_OutFile,"baseframe {\r\n");

		int boneCount=(int)_BoneList.size();
		for (int i=0;i<boneCount;++i)
		{
			BoneInfo& info=_BoneList.at(i);
			Point3 pos=info.Pos;
			if (info.Node)
			{
				//ĳ����ͷ�����ŵ���Ҫ�˸��ڵ������
				IGameNode * parentNode=info.Node->GetNodeParent();
				if (parentNode)
				{
					Point3 scale=parentNode->GetLocalTM(_TvToDump).Scaling();
					pos.x*=scale.x;
					pos.y*=scale.y;
					pos.z*=scale.z;
				}
			}
			Quat rot=info.Rot;

			if (rot.w<0)
			{
				rot=-rot;
			}

			fprintf(_OutFile,"\t( %f %f %f ) ( %f %f %f )\r\n",
				pos.x,pos.y,pos.z,
				rot.x,rot.y,rot.z);
		}

		fprintf(_OutFile,"}\r\n\r\n");
	}

	void DumpFrames() 
	{
		TimeValue start=pIgame->GetSceneStartTime();
		TimeValue end=pIgame->GetSceneEndTime();
		TimeValue ticks=pIgame->GetSceneTicks();

		for (TimeValue tv = start; tv <= end; tv += ticks)
		{
			int frameNum=tv/ticks;
			fprintf(_OutFile,"frame %d {\r\n",frameNum);

			int boneCount=(int)_BoneList.size();
			//orgin��ͷ������
			for (int b=1;b<boneCount;++b)
			{
				BoneInfo& info=_BoneList.at(b);
				DumpPosRot(info.Node,tv,info.Flag);
			}

			fprintf(_OutFile,"}\r\n\r\n");
		}

	}

	void DumpPosRot( IGameNode * pGameNode,TimeValue tv,int flag) 
	{
		IGameObject * obj = pGameNode->GetIGameObject();
		if ((obj->GetIGameType()==IGameObject::IGAME_BONE&&
			(_HelperObject||obj->GetMaxObject()->SuperClassID()!=HELPER_CLASS_ID))||
			(_HelperObject&&obj->GetIGameType()==IGameObject::IGAME_HELPER))
		{
			GMatrix mat;
			IGameNode * parentNode=pGameNode->GetNodeParent();
			if (parentNode)
			{
				mat=parentNode->GetWorldTM(tv);
				mat=ToRightHand(mat);
				mat=mat.Inverse();
			}
			mat=pGameNode->GetWorldTM(tv)*mat;
			mat=ToRightHand(mat);
			Point3 pos=mat.Translation();
			//ĳ����ͷ�����ŵ���Ҫ�˸��ڵ������
			if (parentNode)
			{
				Point3 scale=parentNode->GetWorldTM(tv).Scaling();
				pos.x*=scale.x;
				pos.y*=scale.y;
				pos.z*=scale.z;
			}
			Quat rot=mat.Rotation();

			if (rot.w<0)
			{
				rot=-rot;
			}

			AnimData(flag, pos, rot);
		}

		pGameNode->ReleaseIGameObject();
	}

	void AnimData( int flag, const Point3 &pos, const Quat &rot ) 
	{
		float data[6];
		int size=0;
		if (flag&eHierarchyFlag_Pos_X)
			data[size++]=pos.x;
		if (flag&eHierarchyFlag_Pos_Y)
			data[size++]=pos.y;
		if (flag&eHierarchyFlag_Pos_Z)
			data[size++]=pos.z;

		if (flag&eHierarchyFlag_Rot_X)
			data[size++]=rot.x;
		if (flag&eHierarchyFlag_Rot_Y)
			data[size++]=rot.y;
		if (flag&eHierarchyFlag_Rot_Z)
			data[size++]=rot.z;
		switch (size)
		{
		case 0:
			break;
		case 1:
			fprintf(_OutFile,"\t%f\r\n",
				data[0]);
			break;
		case 2:
			fprintf(_OutFile,"\t%f %f\r\n",
				data[0],data[1]);
			break;
		case 3:
			fprintf(_OutFile,"\t%f %f %f\r\n",
				data[0],data[1],data[2]);
			break;
		case 4:
			fprintf(_OutFile,"\t%f %f %f %f\r\n",
				data[0],data[1],data[2],
				data[3]);
			break;
		case 5:
			fprintf(_OutFile,"\t%f %f %f %f %f\r\n",
				data[0],data[1],data[2],
				data[3],data[4]);
			break;
		case 6:
			fprintf(_OutFile,"\t%f %f %f %f %f %f\r\n",
				data[0],data[1],data[2],
				data[3],data[4],data[5]);
			break;
		}
	}
};



class md5animation_exporterClassDesc : public ClassDesc2 
{
public:
	virtual int IsPublic() 							{ return TRUE; }
	virtual void* Create(BOOL /*loading = FALSE*/) 		{ return new md5animation_exporter(); }
	virtual const TCHAR *	ClassName() 			{ return GetString(IDS_CLASS_NAME); }
	virtual SClass_ID SuperClassID() 				{ return SCENE_EXPORT_CLASS_ID; }
	virtual Class_ID ClassID() 						{ return md5animation_exporter_CLASS_ID; }
	virtual const TCHAR* Category() 				{ return GetString(IDS_CATEGORY); }

	virtual const TCHAR* InternalName() 			{ return _T("md5animation_exporter"); }	// returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE HInstance() 					{ return hInstance; }					// returns owning module handle
	

};


ClassDesc2* Getmd5animation_exporterDesc() { 
	static md5animation_exporterClassDesc md5animation_exporterDesc;
	return &md5animation_exporterDesc; 
}





INT_PTR CALLBACK md5animation_exporterOptionsDlgProc(HWND hWnd,UINT message,WPARAM,LPARAM lParam) {
	static md5animation_exporter* imp = nullptr;

	switch(message) {
		case WM_INITDIALOG:
			imp = (md5animation_exporter *)lParam;
			CenterWindow(hWnd,GetParent(hWnd));
			return TRUE;

		case WM_CLOSE:
			EndDialog(hWnd, 0);
			return 1;
	}
	return 0;
}


//--- md5animation_exporter -------------------------------------------------------
md5animation_exporter::md5animation_exporter()
	:_IncludeBounds(TRUE),
	_HelperObject(FALSE),
	_DoomVersion(TRUE)
{

}

md5animation_exporter::~md5animation_exporter() 
{

}

int md5animation_exporter::ExtCount()
{
	#pragma message(TODO("Returns the number of file name extensions supported by the plug-in."))
	return 1;
}

const TCHAR *md5animation_exporter::Ext(int /*i*/)
{		
	#pragma message(TODO("Return the 'i-th' file name extension (i.e. \"3DS\")."))
	return _T("md5anim");
}

const TCHAR *md5animation_exporter::LongDesc()
{
	#pragma message(TODO("Return long ASCII description (i.e. \"Targa 2.0 Image File\")"))
	return _T("md5 anim file");
}
	
const TCHAR *md5animation_exporter::ShortDesc() 
{			
	#pragma message(TODO("Return short ASCII description (i.e. \"Targa\")"))
	return _T("md5anim");
}

const TCHAR *md5animation_exporter::AuthorName()
{			
	#pragma message(TODO("Return ASCII Author name"))
	return _T("yaorongzhen");
}

const TCHAR *md5animation_exporter::CopyrightMessage() 
{	
	#pragma message(TODO("Return ASCII Copyright message"))
	return _T("");
}

const TCHAR *md5animation_exporter::OtherMessage1() 
{		
	//TODO: Return Other message #1 if any
	return _T("");
}

const TCHAR *md5animation_exporter::OtherMessage2() 
{		
	//TODO: Return other message #2 in any
	return _T("");
}

unsigned int md5animation_exporter::Version()
{				
	#pragma message(TODO("Return Version number * 100 (i.e. v3.01 = 301)"))
	return 100;
}

void md5animation_exporter::ShowAbout(HWND /*hWnd*/)
{			
	// Optional
}

BOOL md5animation_exporter::SupportsOptions(int /*ext*/, DWORD /*options*/)
{
	#pragma message(TODO("Decide which options to support.  Simply return true for each option supported by each Extension the exporter supports."))
	return TRUE;
}


int	md5animation_exporter::DoExport(const TCHAR* name, ExpInterface* ei, Interface* i, BOOL suppressPrompts, DWORD options)
{
	#pragma message(TODO("Implement the actual file Export here and"))

	int result = TRUE;
	if(!suppressPrompts)
	{
		result = DialogBoxParam(hInstance, 
				MAKEINTRESOURCE(IDD_PANEL), 
				GetActiveWindow(), 
				md5animation_exporterOptionsDlgProc, (LPARAM)this);
		result=TRUE;
		//if (result>0)
		{
			MyErrorProc pErrorProc;
			SetErrorCallBack(&pErrorProc);

			pIgame=GetIGameInterface();

			IGameConversionManager * cm = GetConversionManager();
			cm->SetCoordSystem(IGameConversionManager::IGAME_MAX);
			pIgame->InitialiseIGame();
			pIgame->SetStaticFrame(0);

			_TvToDump=0;

			SimpleFile outFile(name,L"wb");
			_OutFile=outFile.File();

			result=SaveMd5Anim(ei,i);

			MessageBox( GetActiveWindow(), name, _T("md5anim finish"), 0 );

			pIgame->ReleaseIGame();
			_OutFile=NULL;
		}
		//else
		//	result=TRUE;
	}
	#pragma message(TODO("return TRUE If the file is exported properly"))
	return result;
}


