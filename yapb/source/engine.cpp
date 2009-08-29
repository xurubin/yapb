//
// Copyright (c) 2003-2009, by Yet Another POD-Bot Development Team.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// $Id: engine.cpp 23 2009-06-16 21:59:33Z jeefo $
//

#include <core.h>

ConVar::ConVar (const char *name, const char *initval, VarType type)
{
   engine->RegisterVariable (name, initval, type, this);
}

void Engine::InitFastRNG (void)
{
   m_divider = (static_cast <uint64_t> (1)) << 32;

   m_rnd[0] = clock ();
   m_rnd[1] = ~(m_rnd[0] + 6);
   m_rnd[1] = GetRandomBase ();
}

uint32_t Engine::GetRandomBase (void)
{
   m_rnd[0] ^= m_rnd[1] << 5;

   m_rnd[0] *= 1664525L;
   m_rnd[0] += 1013904223L;

   m_rnd[1] *= 1664525L;
   m_rnd[1] += 1013904223L;

   m_rnd[1] ^= m_rnd[0] << 3;

   return m_rnd[0];
}

float Engine::RandomFloat (float low, float high)
{
   if (low >= high)
      return low;

   double rnd = GetRandomBase ();

   rnd *= static_cast <double> (high - low);
   rnd /= m_divider - 1.0;

   return static_cast <float> (rnd + static_cast <double> (low));
}

int Engine::RandomInt (int low, int high)
{
   if (low >= high)
      return low;

   double rnd = GetRandomBase ();

   rnd *= static_cast <double> (high - low) + 1;
   rnd /= m_divider;

   return static_cast <int> (rnd + static_cast <double> (low));
}

void Engine::RegisterVariable (const char *variable, const char *value, VarType varType, ConVar *self)
{
   VarPair newVariable;

   newVariable.reg.name = const_cast <char *> (variable);
   newVariable.reg.string = const_cast <char *> (value);

   int engineFlags = FCVAR_EXTDLL;

   if (varType == VARTYPE_NORMAL)
      engineFlags |= FCVAR_SERVER;
   else if (varType == VARTYPE_READONLY)
      engineFlags |= FCVAR_SERVER | FCVAR_SPONLY | FCVAR_PRINTABLEONLY;
   else if (varType == VARTYPE_PASSWORD)
      engineFlags |= FCVAR_PROTECTED;

   newVariable.reg.flags = engineFlags;
   newVariable.self = self;

   memcpy (&m_regVars[m_regCount], &newVariable, sizeof (VarPair));
   m_regCount++;
}

void Engine::PushRegisteredConVarsToEngine (void)
{
   for (int i = 0; i < m_regCount; i++)
   {
      VarPair *ptr = &m_regVars[i];

      if (ptr == NULL)
         break;

      g_engfuncs.pfnCVarRegister (&ptr->reg);
      ptr->self->m_eptr = g_engfuncs.pfnCVarGetPointer (ptr->reg.name);
   }
}

void Engine::GetGameConVarsPointers (void)
{
   m_gameVars[GVAR_C4TIMER] = g_engfuncs.pfnCVarGetPointer ("mp_c4timer");
   m_gameVars[GVAR_BUYTIME] = g_engfuncs.pfnCVarGetPointer ("mp_buytime");
   m_gameVars[GVAR_FRIENDLYFIRE] = g_engfuncs.pfnCVarGetPointer ("mp_friendlyfire");
   m_gameVars[GVAR_ROUNDTIME] = g_engfuncs.pfnCVarGetPointer ("mp_roundtime");
   m_gameVars[GVAR_FREEZETIME] = g_engfuncs.pfnCVarGetPointer ("mp_freezetime");
   m_gameVars[GVAR_FOOTSTEPS] = g_engfuncs.pfnCVarGetPointer ("mp_footsteps");
   m_gameVars[GVAR_GRAVITY] = g_engfuncs.pfnCVarGetPointer ("sv_gravity");
   m_gameVars[GVAR_DEVELOPER] = g_engfuncs.pfnCVarGetPointer ("developer");

   // if buytime is null, just set it to round time
   if (m_gameVars[GVAR_BUYTIME] == NULL)
      m_gameVars[GVAR_BUYTIME] = m_gameVars[3];
}

const Vector &Engine::GetGlobalVector (GlobalVector id)
{
   switch (id)
   {
   case GLOBALVECTOR_FORWARD:
      return g_pGlobals->v_forward;

   case GLOBALVECTOR_RIGHT:
      return g_pGlobals->v_right;

   case GLOBALVECTOR_UP:
      return g_pGlobals->v_up;
   }
   return Vector::GetNull ();
}

void Engine::SetGlobalVector (GlobalVector id, const Vector &newVector)
{
   switch (id)
   {
   case GLOBALVECTOR_FORWARD:
      g_pGlobals->v_forward = newVector;
      break;

   case GLOBALVECTOR_RIGHT:
      g_pGlobals->v_right = newVector;
      break;

   case GLOBALVECTOR_UP:
      g_pGlobals->v_up = newVector;
      break;
   }
   InternalAssert (0);
}

void Engine::BuildGlobalVectors (const Vector &on)
{
   on.BuildVectors (&g_pGlobals->v_forward, &g_pGlobals->v_right, &g_pGlobals->v_up);
}

bool Engine::IsFootstepsOn (void)
{
   return m_gameVars[GVAR_FOOTSTEPS]->value > 0;
}

float Engine::GetC4TimerTime (void)
{
   return m_gameVars[GVAR_C4TIMER]->value;
}

float Engine::GetBuyTime (void)
{
   return m_gameVars[GVAR_BUYTIME]->value;
}

float Engine::GetRoundTime (void)
{
   return m_gameVars[GVAR_ROUNDTIME]->value;
}

float Engine::GetFreezeTime (void)
{
   return m_gameVars[GVAR_FREEZETIME]->value;
}

int Engine::GetGravity (void)
{
   return static_cast <int> (m_gameVars[GVAR_GRAVITY]->value);
}

int Engine::GetDeveloperLevel (void)
{
   return static_cast <int> (m_gameVars[GVAR_DEVELOPER]->value);
}

bool Engine::IsFriendlyFireOn (void)
{
   return m_gameVars[GVAR_FRIENDLYFIRE]->value > 0;
}

void Engine::PrintServer (const char *format, ...)
{
   static char buffer[1024];
   va_list ap;

   va_start (ap, format);
   vsprintf (buffer, format, ap);
   va_end (ap);

   buffer[strlen (buffer) - 1] = '\n';
   g_engfuncs.pfnServerPrint (buffer);
}

int Engine::GetMaxClients (void)
{
   return g_pGlobals->maxClients;
}

float Engine::GetTime (void)
{
   return g_pGlobals->time;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// CLIENT
//////////////////////////////////////////////////////////////////////////
float Client::GetShootingConeDeviation (const Vector &pos)
{
   engine->BuildGlobalVectors (GetViewAngles ());

   return g_pGlobals->v_forward | (pos - GetHeadOrigin ()).Normalize ();
}

bool Client::IsInViewCone (const Vector &pos)
{
   engine->BuildGlobalVectors (GetViewAngles ());

   return ((pos - GetHeadOrigin ()).Normalize () | g_pGlobals->v_forward) >= cosf (Math::DegreeToRadian ((GetFOV () > 0.0 ? GetFOV () : 90.0) * 0.5));
}

bool Client::IsVisible (const Vector &pos)
{
   TraceResult tr;
   TraceLine (GetHeadOrigin (), pos, true, true, m_ent, &tr);

   return ! tr.flFraction != 1.0;
}
