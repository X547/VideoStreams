#pragma once

#include <Referenceable.h>
#include <private/shared/AutoDeleter.h>
#include <private/shared/AutoDeleterOS.h>
#include <locks.h>

#include <map>


class _EXPORT MappedArea: public BReferenceable
{
private:
	friend class AreaCloner;

	area_id fSrcArea;
	AreaDeleter fArea;
	uint8* fAdr;

	MappedArea(area_id srcArea, bool transferOwnership = false);
	virtual ~MappedArea();

public:
	area_id GetArea() {return fArea.Get();}
	uint8 *GetAddress() {return fAdr;}
};


class _EXPORT AreaCloner {
private:
	friend class MappedArea;

	static mutex fLock;
	static std::map<area_id, MappedArea*> fMappedAreas;

	AreaCloner();

public:
	static BReference<MappedArea> Map(area_id srcArea, bool transferOwnership = false);
	static void Unmap(area_id area);
};
