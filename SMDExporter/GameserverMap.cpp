/**
 * ______________________________________________________
 * This file is part of ko-smd-generator project.
 * 
 * @author       Mustafa Kemal Gılor <mustafagilor@gmail.com> (2016)
 * .
 * SPDX-License-Identifier:	MIT
 * ______________________________________________________
 */


#include "stdafx.h"
#include "GameserverMap.h"
#include "tstring.h"
#include <vector>
#include <string>
#include <string.h>


CGameserverMap::CGameserverMap() :m_nXRegion(-1), m_nZRegion(-1),m_ppEvents(nullptr),m_iMapSize(0)
{}

CGameserverMap::~CGameserverMap()
{
	if (m_ppEvents)
	{
		/// Deallocate event array
		for (auto i = 0; i < GetMapSize(); i++)
			delete[] m_ppEvents[i];
		delete[] m_ppEvents;
	}

	Cleanup();
}

void CGameserverMap::Initialize(bool isGSMD)
{
	m_nXRegion = m_nZRegion = static_cast<int>(m_ObjectPostData.GetShapeMgr()->Width() / VIEW_DISTANCE) + 1;
	m_iMapSize = static_cast<int>(GetMapSize() * fUnitDistance);

	/* Map size should always be even.*/
	ASSERT(m_iMapSize % 2 == 0);

	/* Initialize event array */
	m_ppEvents = new short*[GetMapSize() +1];
	for (auto i = 0; i < GetMapSize() + 1; i++)
		m_ppEvents[i] = new short[GetMapSize() + 1];

	if (!isGSMD)
	{
		/* Set default as movable */
		for (int x = 0; x < GetMapSize() + 1; x++)
		{
			for (int z = 0; z < GetMapSize() + 1; z++)
			{
				m_ppEvents[x][z] = 1;
			}
		}

		GenerateObjectEvents();
		// tile data
		GenerateMoveTable();
		m_GSMDHeader.VERSION_HI = GAMESERVER_MAP_VERSION_HI;
		m_GSMDHeader.VERSION_LO = GAMESERVER_MAP_VERSION_LO;
		m_GSMDHeader.szAuthor = "default author";
		//localtime()
		time_t time = std::time(nullptr);
		/* Adjusted to local time*/
		// mktime(localtime(&time));
		m_GSMDHeader.ulCreationDate = time;
		//m_GSMDHeader.szMapName = 
		Cleanup();
	}
}

void CGameserverMap::Cleanup()
{
	/* Release the unrequired shape data */
	m_ObjectPostData.DoCleanup();
}

bool CGameserverMap::LoadObjectPostData(const std::string& szOpdFile) { return m_ObjectPostData.Load(szOpdFile); }
bool CGameserverMap::LoadTerrainData(const std::string& szGtdFile) { return m_TerrainData.Load(szGtdFile); }



CGameserverMap* CGameserverMap::Load(const std::string& szOpdFile, const std::string& szGtdFile)
{
	CGameserverMap * pGameserverMap = new CGameserverMap();

	if (pGameserverMap->LoadObjectPostData(string_format(OPD_PATH "%s", szOpdFile.c_str())) && pGameserverMap->LoadTerrainData(string_format(GTD_PATH "%s", szGtdFile.c_str())))
	{
		if (!pGameserverMap->Valid())
		{
			delete pGameserverMap;
			CGameserverMap::OnInvalidMap();
			return nullptr;
		}
		pGameserverMap->Initialize();
		std::string mapName(szOpdFile.begin(), szOpdFile.end() - 4);
		// set map name
		pGameserverMap->m_GSMDHeader.szMapName = string_format("%s", mapName.c_str());
		return pGameserverMap;
	}

	/// Delete the allocated memory
	delete pGameserverMap;
	TRACE("CGameserverMap::Load() - Map load failed. (opd file : %s, gtd file %s)", szOpdFile.c_str(), szGtdFile.c_str());
	return nullptr;
}

bool CGameserverMap::Valid()
{
	if (GetMapSize() * fUnitDistance != m_ObjectPostData.GetShapeMgr()->Width())
	{
		TRACE("CGameserverMap::Valid() - Width validation failed. (Map size : %g, Unit distance : %g, Width : %g)", GetMapSize(), fUnitDistance, m_ObjectPostData.GetShapeMgr()->Width());
		return false;
	}
	if (GetMapSize() * fUnitDistance != m_ObjectPostData.GetShapeMgr()->Height())
	{
		TRACE("CGameserverMap::Valid() - Height validation failed. (Map size : %g, Unit distance : %g, Height : %g)", GetMapSize(), fUnitDistance, m_ObjectPostData.GetShapeMgr()->Height());
		return false;
	}
	/*if (m_nXRegion < 0 || m_nZRegion < 0)
	return false;*/

	return true;
}





bool CGameserverMap::ValidatePosition(__Vector3& vPos) { return ValidatePosition(vPos.x, vPos.y, vPos.z); }

int CGameserverMap::GetEvent(__Vector3 vPos) const { return GetEvent(vPos.x, vPos.z); }
int CGameserverMap::GetEvent(const float fPosX, const float fPosZ) const
{
	auto fTileX = Coordinate2Tile(fPosX);
	auto fTileZ = Coordinate2Tile(fPosZ);
	if (fTileX < 0 || fTileX > GetMapSize() || fTileZ < 0 || fTileZ > GetMapSize())
		return -1;

	/*
		Retrieve tile number
	*/
	return m_ppEvents[fTileX][fTileZ];
}



bool CGameserverMap::CheckCollision(__Vector3& vPosOld, __Vector3 & vPosNew)
{
	//vPosNew.y = m_TerrainData.GetHeight(vPosNew.x, vPosNew.z);
	//	vPosOld.y = m_TerrainData.GetHeight(vPosOld.x, vPosOld.z);
	TRACE("CGameserverMap::CheckCollision() - vPosOld : (%g,%g,%g) vPosNew (%g,%g,%g)", vPosOld.x, vPosOld.y, vPosOld.z, vPosNew.x, vPosNew.y, vPosNew.z);

	__Vector3	vDir = vPosNew - vPosOld;
	//float fSpeed = vDir.Magnitude();
	vDir.Normalize();

	return GetShapeMgr()->CheckCollision(vPosOld, vPosNew, vDir);
}

bool CGameserverMap::ValidatePosition(const float fX, const float fY, const float fZ)
{
	return (fX >= 0 && fX <= m_iMapSize && fZ >= 0 && fZ <= m_iMapSize);
}

float CGameserverMap::GetHeightBy2DPos(const float fX, const float fZ)
{
	return m_TerrainData.GetHeight(Coordinate2Tile(fX), Coordinate2Tile(fZ));
}

_OBJECT_EVENT* CGameserverMap::GetObjectEvent(int iObjectID)
{
	return m_ObjectEvents.GetData(iObjectID);
}

#ifdef GSMD_TOOL

void CGameserverMap::GenerateObjectEvents()
{
	auto shape_mgr = m_ObjectPostData.GetShapeMgr();
	//ASSERT(shape_mgr);
	std::vector<CN3ShapeEx*> & shapes = shape_mgr->GetShapes();
	foreach_auto(shapes)
	{
		if (elem->GetEventID() > 0)
		{
			_OBJECT_EVENT * pObjectEvent = new _OBJECT_EVENT;
			pObjectEvent->fPosX = elem->GetVPosition().x;
			pObjectEvent->fPosY = elem->GetVPosition().y;
			pObjectEvent->fPosZ = elem->GetVPosition().z;
			pObjectEvent->byNation = elem->GetBelongID();
			pObjectEvent->byType = elem->GetEventType();
			pObjectEvent->sAssociatedNpcID = elem->GetNpcID();
			pObjectEvent->byStatus = elem->GetNpcStatus();
			pObjectEvent->sObjectID = elem->GetEventID();
			pObjectEvent->isAlive = 1;

			ASSERT(pObjectEvent->sObjectID >= 0);
			if (!m_ObjectEvents.PutData(pObjectEvent->sObjectID, pObjectEvent))
				delete pObjectEvent;
		}

	}
}

void CGameserverMap::GenerateMoveTable()
{
	m_TerrainData.MakeMoveTable(m_ppEvents);
	m_ObjectPostData.GetShapeMgr()->MakeMoveTable(m_ppEvents);
}

// Helper: Write warp list section from external warps/<mapname>.txt file.
// File format (one warp per line, # for comments):
//   warpID  zone  x  y  z  radius  nation  pay  name...
// Server expects: int regeneCount(=0) + int warpCount + N x 320 bytes _WARP_INFO struct (pack=1)
static void WriteWarpsForMap(FILE* fp, const std::string& szFileName)
{
	// Always write regeneCount=0 first (server LoadRegeneEvent reads this before LoadWarpList)
	int regeneCount = 0;
	fwrite(&regeneCount, 4, 1, fp);

	// Extract base map name from szFileName (e.g. ".\gsmd\moradon_0826.gsmd" -> "moradon_0826")
	std::string baseName = szFileName;
	size_t lastSlash = baseName.find_last_of("/\\");
	if (lastSlash != std::string::npos) baseName = baseName.substr(lastSlash + 1);
	size_t dotPos = baseName.find_last_of('.');
	if (dotPos != std::string::npos) baseName = baseName.substr(0, dotPos);

	std::string warpFile = "warps/" + baseName + ".txt";
	FILE* wfp = nullptr;
	fopen_s(&wfp, warpFile.c_str(), "r");

	if (!wfp)
	{
		int warpCount = 0;
		fwrite(&warpCount, 4, 1, fp);
		printf("[Warps] No warps/%s.txt - empty warp list\n", baseName.c_str());
		return;
	}

	// Read all warps into a buffer
	std::vector<std::vector<unsigned char>> warps;
	char line[1024];
	while (fgets(line, sizeof(line), wfp))
	{
		if (line[0] == '#' || line[0] == '\n' || line[0] == '\r' || line[0] == 0) continue;

		int warpID = 0, zone = 0, nation = 0, pay = 0;
		float x = 0, y = 0, z = 0, radius = 50;
		char name[256] = {0};

		// Parse: warpID zone x y z radius nation pay name...
		int parsed = sscanf_s(line, "%d %d %f %f %f %f %d %d %255[^\r\n]",
			&warpID, &zone, &x, &y, &z, &radius, &nation, &pay,
			name, (unsigned)sizeof(name));

		if (parsed < 6) continue;

		// Build 320-byte _WARP_INFO struct (pack=1, exact layout)
		std::vector<unsigned char> buf(320, 0);
		// [0..1]    sWarpID (short)
		*(short*)&buf[0] = (short)warpID;
		// [2..33]   strWarpName[32]
		strncpy_s((char*)&buf[2], 32, name, 31);
		// [34..289] strAnnounce[256] -> already zeros
		// [290..291] sUnk0 -> 0
		// [292..295] dwPay (uint32)
		*(unsigned int*)&buf[292] = (unsigned int)pay;
		// [296..297] sZone (short)
		*(short*)&buf[296] = (short)zone;
		// [298..299] sUnk1 -> 0
		// [300..303] fX
		*(float*)&buf[300] = x;
		// [304..307] fY
		*(float*)&buf[304] = y;
		// [308..311] fZ
		*(float*)&buf[308] = z;
		// [312..315] fR (radius)
		*(float*)&buf[312] = radius;
		// [316..317] sNation (short)
		*(short*)&buf[316] = (short)nation;
		// [318..319] sUnk2 -> 0

		warps.push_back(buf);
	}
	fclose(wfp);

	int warpCount = (int)warps.size();
	fwrite(&warpCount, 4, 1, fp);
	for (auto& w : warps)
		fwrite(w.data(), 320, 1, fp);

	printf("[Warps] Loaded %d warps for %s from %s\n",
		warpCount, baseName.c_str(), warpFile.c_str());
}

bool CGameserverMap::SaveGameserverMapData(const std::string& szFileName)
{

	FILE * fp ,* fp2;
	std::string objevt = string_format("%sobjects.txt", szFileName.c_str());
	fopen_s(&fp2, objevt.c_str(), "wb+");
	fopen_s(&fp, szFileName.c_str(), "wb+");
	if (!fp || !fp2)
		return false;

	/* Write map version */
	//fwrite(&GAMESERVER_MAP_VERSION_HI, sizeof(uint8), 1, fp);
	//fwrite(&GAMESERVER_MAP_VERSION_LO, sizeof(uint8), 1, fp);
	// HEADER DISABLED: NTTGame 2444 SMD format does not use a header.
	// m_GSMDHeader.WriteHeader(fp);

	/// Write terrain data
	m_TerrainData.SaveToFilestream(fp);
	/// Save collision data
	m_ObjectPostData.GetShapeMgr()->SaveCollisionData(fp);
	// FIX: Server reads 24 bytes per _OBJECT_EVENT but we wrote 19 bytes - 5 byte mismatch
	// per event corrupting move table & warp data positions. Server actually loads object
	// events from K_OBJECTPOS DB table, the SMD ones are throwaway. Write count=0 to skip
	// the section entirely - server's for-loop won't execute, file pointer stays clean.
	int nObjectEventCount = 0;
	fwrite(&nObjectEventCount, 4, 1, fp);
	// (Original 19-byte event writes removed; objects.txt debug dump below still uses m_ObjectEvents)

	for (int i = 0; i < 10; i++)
	{
		switch(i)
		{
		case 0:
			fprintf_s(fp2, "< BIND POINTS >\n");
			break;
		case 1:
			fprintf_s(fp2, "< TYPE 1 GATES >\n");
			break;
		case 2:
			fprintf_s(fp2, "< TYPE 2 GATES >\n");
			break;
		case 3:
			fprintf_s(fp2, "< GATE LEVERS >\n");
			break;
			break;
		case 4:
			fprintf_s(fp2, "< FLAGS >\n");
			break;
		case 5:
			fprintf_s(fp2, "< WARP GATES >\n");
			break;
		case 6:
			fprintf_s(fp2, "< BARRICADES >\n");
			break;
		case 7:
			fprintf_s(fp2, "< BIND POINTS (remove) >\n");
			break;
		case 8:
			fprintf_s(fp2, "< MAGIC ANVILS >\n");
			break;
		case 9:
			fprintf_s(fp2, "< ARTIFACTS >\n");
			break;
		}
		{
			foreach_stlmap(itr, m_ObjectEvents)
			{
				std::string object_name = "unknown";
				_OBJECT_EVENT * pObjectEvent = itr->second;
				for (const auto & q : m_ObjectPostData.GetShapeMgr()->GetShapes()) {
					if (q->GetNpcID() == itr->second->sAssociatedNpcID) {
						object_name = q->GetName();
					}
				}
				//m_ObjectPostData.
				if (pObjectEvent->byType != i)
					continue;
				fprintf_s(fp2, "Object ID %d, Associated NPC :%d, Type : %d, Nation %d, Status : %d, POS(%g,%g,%g), Name : %s \n", pObjectEvent->sObjectID, pObjectEvent->sAssociatedNpcID, pObjectEvent->byType, pObjectEvent->byNation, pObjectEvent->byStatus, pObjectEvent->fPosX, pObjectEvent->fPosY, pObjectEvent->fPosZ, object_name.c_str());
			}
		}

	
	}

	fprintf_s(fp2, "< FULL LIST >\n");
	{
		foreach_stlmap(itr, m_ObjectEvents)
		{

			_OBJECT_EVENT * pObjectEvent = itr->second;
			fprintf_s(fp2, "Object ID %d, Associated NPC :%d, Type : %d, Nation %d, Status : %d, POS(%g,%g,%g) \n", pObjectEvent->sObjectID, pObjectEvent->sAssociatedNpcID, pObjectEvent->byType, pObjectEvent->byNation, pObjectEvent->byStatus, pObjectEvent->fPosX, pObjectEvent->fPosY, pObjectEvent->fPosZ);
		}
	}
	

	/// Save tile data
	SaveMoveTableToStream(fp);

	// NEW: Append regene events (count=0) + warp list section
	// Server's SMDFile::LoadMap reads these AFTER move table when bLoadWarpsAndRegeneEvents=true
	WriteWarpsForMap(fp, szFileName);

	fclose(fp);
	fclose(fp2);
	return true;
}

void CGameserverMap::LoadAndWriteExternalTile(const std::string& szFileName)
{
	FILE * fp;

	fopen_s(&fp, szFileName.c_str(), "rb");

	if (!fp)
	{
		printf("external tile file %s could not be found.\n", szFileName.c_str());
		return;
	}

	short ** ppTileTable = new short*[GetMapSize() + 1];
	for (auto i = 0; i < GetMapSize() + 1; i++)
		ppTileTable[i] = new short[GetMapSize() + 1];



		for (auto x = 0; x < GetMapSize() + 1; x++)
			for (auto z = 0; z < GetMapSize() + 1; z++)
			{
				fread_s(&ppTileTable[x][z], sizeof(short), sizeof(short), 1, fp);
				
			//	fwrite(&ppTileTable[x][z], sizeof(short), 1, fp);
			}

		/* Sync tile table*/
		for (auto x = 0; x < GetMapSize() + 1; x++)
			for (auto z = 0; z < GetMapSize() + 1; z++)
			{
				if(ppTileTable[x][z] > 1)
				{
					// Copy only custom tiles
					m_ppEvents[x][z] = ppTileTable[x][z];
					//printf("PATCH custom tile %d on position %d,%d!\n", ppTileTable[x][z], x, z);
				}
			}

		/* Deallocate */
		for (auto i = 0; i < GetMapSize() + 1; i++)
			delete[] ppTileTable[i];
		delete[] ppTileTable;
}

bool CGameserverMap::SaveMoveTable(const std::string & szFileName)
{
	FILE * fp;
	fopen_s(&fp, szFileName.c_str(), "w+");

	if (nullptr == fp)
		return false;
	SaveMoveTableToStream(fp);

	fclose(fp);
	return true;
}

bool CGameserverMap::SaveMoveTableToStream(FILE* fp)
{

	for (auto x = 0; x < GetMapSize() + 1; x++)
		for (auto z = 0; z < GetMapSize() + 1; z++)
		{
			fwrite(&m_ppEvents[x][z], sizeof(short), 1, fp);
		}
	return true;
}

#endif

bool CGameserverMap::LoadGameserverMapData(const std::string& szFileName)
{
	FILE * fp;
	fopen_s(&fp, szFileName.c_str(), "rb");
	if (nullptr == fp)
	{
		TRACE("CGameserverMap::LoadGameserverMapData() - The file %s does not exist.", szFileName.c_str());
		return false;
	}
	
	m_GSMDHeader.ReadHeader(fp);
	/* Read map version */
	/*fread_s(&VERSION_HI, 1, 1, 1, fp);
	fread_s(&VERSION_LO, 1, 1, 1, fp);*/
	
	/// Load terrain data
	m_TerrainData.LoadFromStream(fp);
	m_ObjectPostData.GetShapeMgr()->LoadCollisionData(fp);

	//printf("%d", ftell(fp));
	int32 nObjectEventCount;
	fread_s(&nObjectEventCount, 4, 4, 1, fp);
	if(nObjectEventCount > 0)
	{
		for (int i = 0; i < nObjectEventCount; i++)
		{
			_OBJECT_EVENT * pObjectEvent = new _OBJECT_EVENT;

			fread_s(&pObjectEvent->sObjectID, sizeof(short), sizeof(short), 1, fp);
			fread_s(&pObjectEvent->sAssociatedNpcID, sizeof(short), sizeof(short), 1, fp);
			fread_s(&pObjectEvent->byType, sizeof(unsigned __int8), sizeof(unsigned __int8), 1, fp);
			fread_s(&pObjectEvent->byNation, sizeof(unsigned __int8), sizeof(unsigned __int8), 1, fp);
			fread_s(&pObjectEvent->byStatus, sizeof(unsigned __int8), sizeof(unsigned __int8), 1, fp);
			fread_s(&pObjectEvent->fPosX, sizeof(float), sizeof(float), 1, fp);
			fread_s(&pObjectEvent->fPosY, sizeof(float), sizeof(float), 1, fp);
			fread_s(&pObjectEvent->fPosZ, sizeof(float), sizeof(float), 1, fp);
			//fread_s(pObjEvent, sizeof(_OBJECT_EVENT), sizeof(_OBJECT_EVENT), 1, fp);
			pObjectEvent->isAlive = 1;
			if (pObjectEvent->sObjectID < 0)
				TRACE("OBJECT FAILURE : offset %d", ftell(fp));
			ASSERT(pObjectEvent->sObjectID >= 0);
			//pObjEvent->
			TRACE("Object event loaded - Index [%d] Nation [%d] Control [%d] Type [%d]",pObjectEvent->sObjectID,pObjectEvent->byNation,pObjectEvent->sAssociatedNpcID,pObjectEvent->byType);
			if (!m_ObjectEvents.PutData(pObjectEvent->sObjectID, pObjectEvent))
			{
				TRACE("CGameserverMap::LoadGameserverMapData() - Duplicate object ID : %d", pObjectEvent->sObjectID);
				delete pObjectEvent;
			}
		}
	}
	
	Initialize(true);
	/* Read tile data */
	for (auto x = 0; x < GetMapSize() + 1; x++)
		for (auto z = 0; z < GetMapSize() + 1; z++)
		{
			fread_s(&m_ppEvents[x][z], sizeof(short), sizeof(short), 1, fp);
		}

	TRACE("CGameserverMap::LoadGameserverMapData() - Loaded map %s, version(%d,%d), author : %s", m_GSMDHeader.szMapName.c_str(), m_GSMDHeader.VERSION_HI, m_GSMDHeader.VERSION_LO, m_GSMDHeader.szAuthor.c_str());
	TRACE("CGameserverMap::LoadGameserverMapData() - File created on %s", asctime(localtime(reinterpret_cast<const long long*>(&m_GSMDHeader.ulCreationDate))));
	//printf()
	return true;
}



void CGameserverMap::OnInvalidMap()
{

	printf("\n ** An error has occurred **\n\n");
	//printf("ERROR: %s is not a valid map file.\n\n", m_MapName.c_str());
	printf("Previously, we ignored all invalid map behaviour, however this only hides\n");
	printf("very real problems - especially with things like AI pathfinding.\n\n");
	printf("This problem is most likely occur with maps tweaked to use a different\n");
	printf("map size. Unfortunately, doing this means data after that (almost everything)\n");
	printf("becomes corrupt, which is known to cause extremely 'unusual' buggy behaviour.\n\n");
	printf("It is recommended you use a map built for this zone, or at the very least,\n"); \
		printf("you should use a map originally built for the same zone size.\n\n");
	ASSERT(0);
}