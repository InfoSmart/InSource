//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef IN_VIEWVECTORS_H
#define IN_VIEWVECTORS_H

#pragma once

//=========================================================
// >> InViewVectors
//=========================================================
class InViewVectors : public CViewVectors
{
public:
	InViewVectors(
		Vector vView,
		Vector vHullMin,
		Vector vHullMax,
		Vector vDuckHullMin,
		Vector vDuckHullMax,
		Vector vDuckView,
		Vector vObsHullMin,
		Vector vObsHullMax,
		Vector vDeadViewHeight,
		Vector vDejectedView 
		) :
			CViewVectors(
			vView,
			vHullMin,
			vHullMax,
			vDuckHullMin,
			vDuckHullMax,
			vDuckView,
			vObsHullMin,
			vObsHullMax,
			vDeadViewHeight 
	)
	{
		m_nVecDejectedView = vDejectedView;
	}

	Vector m_nVecDejectedView;
};

//=========================================================
// Aquí se define los distintos puntos de vista.
//
// Se pueden acceder a ellos con: 
// InRules->GetInViewVectors()->
//=========================================================
static InViewVectors g_InViewVectors(
	Vector( 0, 0, 68 ),       // VEC_VIEW (m_vView) 

	Vector( -16, -16, 0 ),	  // VEC_HULL_MIN (m_vHullMin)
	Vector( 16, 16, 62 ),	  // VEC_HULL_MAX (m_vHullMax)

	Vector( -16, -16, 0 ),	  // VEC_DUCK_HULL_MIN (m_vDuckHullMin)
	Vector( 16, 16, 36 ),	  // VEC_DUCK_HULL_MAX	(m_vDuckHullMax)
	Vector( 0, 0, 28 ),		  // VEC_DUCK_VIEW		(m_vDuckView)

	Vector( -10, -10, -10 ),  // VEC_OBS_HULL_MIN	(m_vObsHullMin)
	Vector( 10, 10, 10 ),	  // VEC_OBS_HULL_MAX	(m_vObsHullMax)

	Vector( 0, 0, 20 ),		  // VEC_DEAD_VIEWHEIGHT (m_vDeadViewHeight)

	Vector( 0, 0, 30 )		  // VEC_DEJECTED_VIEWHEIGHT (vDejectedView)
);

//=========================================================
// Constantes
//=========================================================

#define VEC_DEJECTED_VIEWHEIGHT InRules->GetInViewVectors()->vDejectedView;

#endif // IN_VIEWVECTORS_H