// Copyright (C) 2005 Laurent Morichetti
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


#include "StdAfx.h"
#include "ACDVirtualCP.h"

#include <setupapi.h>
#include <cfgmgr32.h>

#include <tchar.h>
#include <Windows.h>

CACDVirtualCP::EnumHelper::ENUMPROC_STATUS
CACDVirtualCP::EnumHelper::Callback (IN CACDVirtualCP* pVCP) {
    ENUMPROC_STATUS status = CBase::Callback (&pVCP->m_Device);

    if (status != ENUMPROC_STATUS_SUCCESS)
	return status;

    //
    // Find out the HID parents's name and hardware instance id.
    //

    SP_DEVINFO_DATA devInfo;
    devInfo.cbSize = sizeof (devInfo);

    if (!SetupDiEnumDeviceInfo (m_hDevInfo, m_dwCurrentIndex, &devInfo))
		return ENUMPROC_STATUS_CONTINUE;

    DEVINST devInst;
    CM_Get_Parent (&devInst, devInfo.DevInst, 0);

	wchar_t buffer[128];
    swprintf_s(buffer, sizeof(buffer), _T(" (HID%d)"), m_dwCurrentIndex);
    buffer [sizeof (buffer) - 1] = '\0';

    ULONG length = 0;
    CM_Get_DevNode_Registry_Property (
	devInst,	    /* devInst */
	CM_DRP_DEVICEDESC,  /* property */
	NULL,		    /* registry data type */
	NULL,		    /* buffer */
	&length,	    /* length */
	0		    /* flags */
	);
    
    const wchar_t* lpcVirtualPanel = L"Virtual Panel on ";
    wchar_t* name = (wchar_t*) malloc(wcslen (lpcVirtualPanel) + length + wcslen(buffer));
    if (!name)
		return ENUMPROC_STATUS_CONTINUE;

    name = L'\0';
    wcscat(name, lpcVirtualPanel);

    wchar_t* start = &name [wcslen(name)];
    if (CM_Get_DevNode_Registry_Property (
	devInst,	    /* devInst */
	CM_DRP_DEVICEDESC,  /* property */
	NULL,		    /* registry data type */
	start,		    /* buffer */
	&length,	    /* length */
	0		    /* flags */
	) != CR_SUCCESS)
	    return ENUMPROC_STATUS_CONTINUE;
    start [length - 1] = '\0';

    wcscat(start, buffer);
    pVCP->m_lpDeviceName = name;

    if (CM_Get_Device_ID (
			devInfo.DevInst,    /* devInst */
			buffer,		    /* buffer */
			sizeof (buffer),    /* length */
			0		    /* flags */
	) != CR_SUCCESS)
	    return ENUMPROC_STATUS_CONTINUE;
    pVCP->m_lpDeviceID = wcsdup (buffer);

    pVCP->InitializeFeatures ();

    m_Array.Add (pVCP);

    return ENUMPROC_STATUS_SUCCESS;
}

CACDVirtualCP::~CACDVirtualCP (void)
{
    delete m_lpDeviceName;
    delete m_lpDeviceID;
}

BOOL
CACDVirtualCP::InitializeFeatures (void)
{
    BOOL bRet = m_Device.GetBrightness (&m_bBrightness);
    if (bRet)
	m_bInitialBrightness = m_bBrightness;
    return bRet;
}

BOOL
CACDVirtualCP::Reset (void)
{
    m_bModified = FALSE;

    if (!CheckBrightness ())
	return InitializeFeatures ();

    m_bBrightness = m_bInitialBrightness;
    return m_Device.SetBrightness (m_bBrightness);
}

BOOL
CACDVirtualCP::Apply (void)
{
    m_bModified = FALSE;

    if (!CheckBrightness ())
	return InitializeFeatures ();

    m_bInitialBrightness = m_bBrightness;
    return TRUE;
}