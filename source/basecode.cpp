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
// $Id:$
//

#include <core.h>

// console variables
ConVar yb_debug ("yb_debug", "0");
ConVar yb_followuser ("yb_followuser", "3");
ConVar yb_followpercent ("yb_followpercent", "40");
ConVar yb_debuggoal ("yb_debuggoal", "-1");
ConVar yb_knifemode ("yb_knifemode", "0");
ConVar yb_commtype ("yb_commtype", "2");
ConVar yb_ecorounds ("yb_ecorounds", "1");
ConVar yb_hardmode ("yb_hardmode", "1");
ConVar yb_walkallow ("yb_walkallow", "1");

ConVar yb_tkpunish ("yb_tkpunish", "1");
ConVar yb_stopbots ("yb_stopbots", "0");
ConVar yb_spraypaints ("yb_spraypaints", "1");
ConVar yb_botbuy ("yb_botbuy", "1");
ConVar yb_msectype ("yb_msectype", "4");

ConVar yb_timersound ("yb_timersound", "0.5");
ConVar yb_timerpickup ("yb_timerpickup", "0.5");
ConVar yb_timergrenade ("yb_timergrenade", "0.5");

ConVar yb_chatterpath ("yb_chatterpath", "sound/radio/bot");
ConVar yb_restrictweapons ("yb_restrictweapons", "ump45;p90;elite;tmp;mac10;m3;xm1014");

int Bot::GetMessageQueue (void)
{
   // this function get the current message from the bots message queue

   int message = m_messageQueue[m_actMessageIndex++];
   m_actMessageIndex &= 0x1f; // wraparound

   return message;
}

void Bot::PushMessageQueue (int message)
{
   // this function put a message into the bot message queue

   if (message == CMENU_SAY)
   {
      // notify other bots of the spoken text otherwise, bots won't respond to other bots (network messages aren't sent from bots)
      int entityIndex = GetIndex ();

      for (int i = 0; i < engine->GetMaxClients (); i++)
      {
         Bot *otherBot = g_botManager->GetBot (i);

         if (otherBot != null && otherBot->pev != pev)
         {
            if (IsAlive (GetEntity ()) == IsAlive (otherBot->GetEntity ()))
            {
               otherBot->m_sayTextBuffer.entityIndex = entityIndex;
               strcpy (otherBot->m_sayTextBuffer.sayText, m_tempStrings);
            }
            otherBot->m_sayTextBuffer.timeNextChat = engine->GetTime () + otherBot->m_sayTextBuffer.chatDelay;
         }
      }
   }
   m_messageQueue[m_pushMessageIndex++] = message;
   m_pushMessageIndex &= 0x1f; // wraparound
}

float Bot::InFieldOfView (Vector destination)
{
   float entityAngle = AngleMod (destination.ToYaw ()); // find yaw angle from source to destination...
   float viewAngle = AngleMod (pev->v_angle.y); // get bot's current view angle...

   // return the absolute value of angle to destination entity
   // zero degrees means straight ahead, 45 degrees to the left or
   // 45 degrees to the right is the limit of the normal view angle
   float absoluteAngle = fabsf (viewAngle - entityAngle);

   if (absoluteAngle > 180.0f)
      absoluteAngle = 360.0f - absoluteAngle;

   return absoluteAngle;
}

bool Bot::IsInViewCone (Vector origin)
{
   // this function returns true if the spatial vector location origin is located inside
   // the field of view cone of the bot entity, false otherwise. It is assumed that entities
   // have a human-like field of view, that is, about 90 degrees.

   return ::IsInViewCone (origin, GetEntity ());
}

bool Bot::CheckVisibility (entvars_t *targetEntity, Vector *origin, uint8_t *bodyPart)
{
   // this function checks visibility of a bot target.

   Vector botHead = EyePosition ();
   TraceResult tr;

   *bodyPart = 0;

   // check for the body
   TraceLine (botHead, targetEntity->origin, true, true, GetEntity (), &tr);

   if (tr.flFraction >= 1.0f)
   {
      *bodyPart |= VISIBILITY_BODY;
      *origin = tr.vecEndPos + Vector (0, 0, 6);
   }

   // check for the head
   TraceLine (botHead, targetEntity->origin + targetEntity->view_ofs, true, true, GetEntity (), &tr);

   if (tr.flFraction >= 1.0f)
   {
      *bodyPart |= VISIBILITY_HEAD;
      *origin = tr.vecEndPos;
   }

   if (*bodyPart != 0)
      return true;

#if 0
   // dimension table
   const int8 dimensionTab[8][3] =
   {
      {1,  1,  1}, { 1,  1, -1},
      {1, -1,  1}, {-1,  1,  1},
      {1, -1, -1}, {-1, -1,  1},
      {-1, 1, -1}, {-1, -1, -1}
   };

   // check the box borders
   for (int i = 7; i >= 0; i--)
   {
      Vector targetOrigin = petargetOrigin->origin + Vector (dimensionTab[i][0] * 24.0, dimensionTab[i][1] * 24.0, dimensionTab[i][2] * 24.0f);

      // check direct line to random part of the player body
      TraceLine (botHead, targetOrigin, true, true, GetEntity (), &tr);

      // check if we hit something
      if (tr.flFraction >= 1.0f)
      {
         *origin = tr.vecEndPos;
         *bodyPart |= Visibility_Other;

         return true;
      }
   }
#else
   // worst case, choose random position in enemy body
   for (int i = 0; i < 5; i++)
   {
      Vector targetOrigin = targetEntity->origin; // get the player origin

      // find the vector beetwen mins and maxs of the player body
      targetOrigin.x += engine->RandomFloat (targetEntity->mins.x, targetEntity->maxs.x);
      targetOrigin.y += engine->RandomFloat (targetEntity->mins.y, targetEntity->maxs.y);
      targetOrigin.z += engine->RandomFloat (targetEntity->mins.z, targetEntity->maxs.z);

      // check direct line to random part of the player body
      TraceLine (botHead, targetOrigin, true, true, GetEntity (), &tr);

      // check if we hit something
      if (tr.flFraction >= 1.0f)
      {
         *origin = tr.vecEndPos;
         *bodyPart |= VISIBILITY_OTHER;

         return true;
      }
   }
#endif
   return false;
}

bool Bot::IsEnemyViewable (edict_t *player)
{
   if (FNullEnt (player))
      return false;

   bool forceTrueIfVisible = false;

   if (IsValidPlayer (pev->dmg_inflictor) && GetTeam (pev->dmg_inflictor) != GetTeam (GetEntity ()) && ::IsInViewCone (EyePosition (), pev->dmg_inflictor))
      forceTrueIfVisible = true;

   if (CheckVisibility (VARS (player), &m_enemyOrigin, &m_visibility) && (IsInViewCone (player->v.origin + Vector (0, 0, 14)) || forceTrueIfVisible))
   {
      m_seeEnemyTime = engine->GetTime ();
      m_lastEnemy = player;
      m_lastEnemyOrigin = player->v.origin;

      return true;
   }
   return false;
}

bool Bot::ItemIsVisible (Vector destination, char *itemName, bool bomb)
{
   TraceResult tr;

   // trace a line from bot's eyes to destination..
   TraceLine (EyePosition (), destination, true, GetEntity (), &tr);

   // check if line of sight to object is not blocked (i.e. visible)
   if (tr.flFraction < 1.0f)
   {
      // check for standard items
      if (tr.flFraction > 0.97 && strcmp (STRING (tr.pHit->v.classname), itemName) == 0)
         return true;

      if (tr.flFraction > (bomb ? 0.93 : 0.95) && strncmp (STRING (tr.pHit->v.classname), "weaponbox", 9) == 0)
         return true;

      if (tr.flFraction > 0.95 && strncmp (STRING (tr.pHit->v.classname), "csdmw_", 6) == 0)
         return true;

      return false;
   }
   return true;
}

bool Bot::EntityIsVisible (Vector dest, bool fromBody)
{
   TraceResult tr;

   // trace a line from bot's eyes to destination...
   TraceLine (fromBody ? pev->origin - Vector (0, 0, 1) : EyePosition (), dest, true, true, GetEntity (), &tr);

   // check if line of sight to object is not blocked (i.e. visible)
   return tr.flFraction >= 1.0f;
}

void Bot::AvoidGrenades (void)
{
   // checks if bot 'sees' a grenade, and avoid it

   edict_t *ent = m_avoidGrenade;

  // check if old pointers to grenade is invalid
   if (FNullEnt (ent))
   {
      m_avoidGrenade = null;
      m_needAvoidGrenade = 0;
   }
   else if ((ent->v.flags & FL_ONGROUND) || (ent->v.effects & EF_NODRAW))
   {
      m_avoidGrenade = null;
      m_needAvoidGrenade = 0;
   }
   ent = null;

   // find all grenades on the map
   while (!FNullEnt (ent = FIND_ENTITY_BY_CLASSNAME (ent, "grenade")))
   {
      // if grenade is invisible don't care for it
      if (ent->v.effects & EF_NODRAW)
         continue;

      // check if visible to the bot
      if (!EntityIsVisible (ent->v.origin) && InFieldOfView (ent->v.origin - EyePosition ()) > pev->fov / 3)
         continue;

#if 0
      // TODO: should be done once for grenade, instead of checking several times
      if (m_skill >= 70 && strcmp (STRING (ent->v.model) + 9, "flashbang.mdl") == 0)
      {
         Vector position = (GetEntityOrigin (ent) - EyePosition ()).ToAngles ();

         // don't look at flashbang
         if (((ent->v.owner == GetEntity () || engine->RandomInt (0, 100) < 85) && !(m_states & State_SeeingEnemy)))
         {
            pev->v_angle.y = AngleNormalize (position.y + 180.0f);
            m_canChooseAimDirection = false;
         }
      }
      else 
#endif
      if (strcmp (STRING (ent->v.model) + 9, "hegrenade.mdl") == 0)
      {
         if (!FNullEnt (m_avoidGrenade))
            return;

         if (GetTeam (ent->v.owner) == GetTeam (GetEntity ()) && ent->v.owner != GetEntity ())
            return;

         if ((ent->v.flags & FL_ONGROUND) == 0)
         {
            float distance = (ent->v.origin - pev->origin).GetLength ();
            float distanceMoved = ((ent->v.origin + ent->v.velocity * m_frameInterval) - pev->origin).GetLength ();

            if (distanceMoved < distance && distance < 500)
            {
               MakeVectors (pev->v_angle);

               Vector dirToPoint = (pev->origin - ent->v.origin).Normalize2D ();
               Vector rightSide = g_pGlobals->v_right.Normalize2D ();

               if ((dirToPoint | rightSide) > 0)
                  m_needAvoidGrenade = -1;
               else
                  m_needAvoidGrenade = 1;

               m_avoidGrenade = ent;
            }
         }
      }
   }
}

bool Bot::IsBehindSmokeClouds (edict_t *ent)
{
   edict_t *pentGrenade = null;
   Vector betweenUs = (ent->v.origin - pev->origin).Normalize ();

   while (!FNullEnt (pentGrenade = FIND_ENTITY_BY_CLASSNAME (pentGrenade, "grenade")))
   {
      // if grenade is invisible don't care for it
      if ((pentGrenade->v.effects & EF_NODRAW) || !(pentGrenade->v.flags & (FL_ONGROUND | FL_PARTIALGROUND)) || strcmp (STRING (pentGrenade->v.model) + 9, "smokegrenade.mdl"))
         continue;

      // check if visible to the bot
      if (!EntityIsVisible (ent->v.origin) && InFieldOfView (ent->v.origin - EyePosition ()) > pev->fov / 3)
         continue;

      Vector betweenNade = (GetEntityOrigin (pentGrenade) - pev->origin).Normalize ();
      Vector betweenResult = ((Vector (betweenNade.y, betweenNade.x, 0) * 150.0f + GetEntityOrigin (pentGrenade)) - pev->origin).Normalize ();

      if ((betweenNade | betweenUs) > (betweenNade | betweenResult))
         return true;
   }
   return false;
}

int Bot::GetBestWeaponCarried (void)
{
   // this function returns the best weapon of this bot (based on personality prefs)

   int *ptr = g_weaponPrefs[m_personality];
   int weaponIndex = 0;
   int weapons = pev->weapons;

   WeaponSelect *weaponTab = &g_weaponSelect[0];

   // take the shield in account
   if (HasShield ())
      weapons |= (1 << WEAPON_SHIELDGUN);

   for (int i = 0; i < Const_NumWeapons; i++)
   {
      if (weapons & (1 << weaponTab[*ptr].id))
         weaponIndex = i;

      ptr++;
   }
   return weaponIndex;
}

int Bot::GetBestSecondaryWeaponCarried (void)
{
   // this function returns the best secondary weapon of this bot (based on personality prefs)

   int *ptr = g_weaponPrefs[m_personality];
   int weaponIndex = 0;
   int weapons = pev->weapons;

   // take the shield in account
   if (HasShield ())
      weapons |= (1 << WEAPON_SHIELDGUN);

   WeaponSelect *weaponTab = &g_weaponSelect[0];

   for (int i = 0; i < Const_NumWeapons; i++)
   {
      int id = weaponTab[*ptr].id;

      if ((weapons & (1 << weaponTab[*ptr].id)) && (id == WEAPON_USP || id == WEAPON_GLOCK18 || id == WEAPON_DEAGLE || id == WEAPON_P228 || id == WEAPON_ELITE || id == WEAPON_FN57))
      {
         weaponIndex = i;
         break;
      }
      ptr++;
   }
   return weaponIndex;
}

bool Bot::RateGroundWeapon (edict_t *ent)
{
   // this function compares weapons on the ground to the one the bot is using

   int hasWeapon = 0;
   int groundIndex = 0;
   int *ptr = g_weaponPrefs[m_personality];

   WeaponSelect *weaponTab = &g_weaponSelect[0];

   for (int i = 0; i < Const_NumWeapons; i++)
   {
      if (strcmp (weaponTab[*ptr].modelName, STRING (ent->v.model) + 9) == 0)
      {
         groundIndex = i;
         break;
      }
      ptr++;
   }

   if (IsRestricted (weaponTab[groundIndex].id) && HasPrimaryWeapon ())
      return false;

   if (groundIndex < 7)
      hasWeapon = GetBestSecondaryWeaponCarried ();
   else
      hasWeapon = GetBestWeaponCarried ();

   return groundIndex > hasWeapon;
}

edict_t *Bot::FindBreakable (void)
{
   // this function checks if bot is blocked by a shoot able breakable in his moving direction

   TraceResult tr;
   TraceLine (pev->origin, pev->origin + (m_destOrigin - pev->origin).Normalize () * 64, false, false, GetEntity (), &tr);

   if (tr.flFraction != 1.0f)
   {
      edict_t *ent = tr.pHit;

      // check if this isn't a triggered (bomb) breakable and if it takes damage. if true, shoot the crap!
      if (IsShootableBreakable (ent))
      {
         m_breakable = tr.vecEndPos;
         return ent;
      }
   }
   TraceLine (EyePosition (), EyePosition () + (m_destOrigin - EyePosition ()).Normalize () * 64, false, false, GetEntity (), &tr);

   if (tr.flFraction != 1.0f)
   {
      edict_t *ent = tr.pHit;

      if (IsShootableBreakable (ent))
      {
         m_breakable = tr.vecEndPos;
         return ent;
      }
   }
   m_breakableEntity = null;
   m_breakable = nullvec;

   return null;
}

void Bot::FindItem (void)
{
   // this function finds Items to collect or use in the near of a bot

   if (GetCurrentTask ()->taskID == TASK_ESCAPEFROMBOMB || GetCurrentTask ()->taskID == TASK_PLANTBOMB)
      return;

   edict_t *ent = null, *pickupItem = null;
   Bot *bot = null;

   bool allowPickup= false;
   float distance, minDistance = 341.0f;

   // don't try to pickup anything while on ladder
   if (IsOnLadder ())
   {
      m_pickupItem = null;
      m_pickupType = PICKTYPE_NONE;

      return;
   }

   if (!FNullEnt (m_pickupItem))
   {
      bool itemExists = false;
      pickupItem = m_pickupItem;

      while (!FNullEnt (ent = FIND_ENTITY_IN_SPHERE (ent, pev->origin, 340.0f)))
      {
         if ((ent->v.effects & EF_NODRAW) || IsValidPlayer (ent->v.owner))
            continue; // someone owns this weapon or it hasn't respawned yet

         if (ent == pickupItem)
         {
            if (ItemIsVisible (GetEntityOrigin (ent), const_cast <char *> (STRING (ent->v.classname)), m_pickupType == PICKTYPE_DROPPEDC4 || m_pickupType == PICKTYPE_PLANTEDC4))
               itemExists = true;

            break;
         }
      }
      if (itemExists)
         return;
      else
      {
         m_pickupItem = null;
         m_pickupType = PICKTYPE_NONE;
      }
   }

   ent = null;
   pickupItem = null;

   PickupType pickupType = PICKTYPE_NONE;

   Vector pickupOrigin = nullvec;
   Vector entityOrigin = nullvec;

   m_pickupItem = null;
   m_pickupType = PICKTYPE_NONE;

   while (!FNullEnt (ent = FIND_ENTITY_IN_SPHERE (ent, pev->origin, 400.0f)))
   {
      allowPickup = false;  // assume can't use it until known otherwise

      if ((ent->v.effects & EF_NODRAW) || ent == m_itemIgnore)
         continue; // someone owns this weapon or it hasn't respawned yet

      entityOrigin = GetEntityOrigin (ent);

      // check if line of sight to object is not blocked (i.e. visible)
      if (ItemIsVisible (entityOrigin, const_cast <char *> (STRING (ent->v.classname))))
      {
         if (strncmp ("hostage_entity", STRING (ent->v.classname), 14) == 0)
         {
            allowPickup = true;
            pickupType = PICKTYPE_HOSTAGE;
         }
         else if (strncmp ("weaponbox", STRING (ent->v.classname), 9) == 0 && strcmp (STRING (ent->v.model) + 9, "backpack.mdl") == 0)
         {
            allowPickup = true;
            pickupType = PICKTYPE_DROPPEDC4;
         }
         else if (strncmp ("weaponbox", STRING (ent->v.classname), 9) == 0 && strcmp (STRING (ent->v.model) + 9, "backpack.mdl") == 0 && !m_isUsingGrenade)
         {
            allowPickup = true;
            pickupType = PICKTYPE_DROPPEDC4;
         }
         else if ((strncmp ("weaponbox", STRING (ent->v.classname), 9) == 0 || strncmp ("armoury_entity", STRING (ent->v.classname), 14) == 0 || strncmp ("csdm", STRING (ent->v.classname), 4) == 0) && !m_isUsingGrenade)
         {
            allowPickup = true;
            pickupType = PICKTYPE_WEAPON;
         }
         else if (strncmp ("weapon_shield", STRING (ent->v.classname), 13) == 0 && !m_isUsingGrenade)
         {
            allowPickup = true;
            pickupType = PICKTYPE_SHIELDGUN;
         }
         else if (strncmp ("item_thighpack", STRING (ent->v.classname), 14) == 0 && GetTeam (GetEntity ()) == TEAM_COUNTER && !m_hasDefuser)
         {
            allowPickup = true;
            pickupType = PICKTYPE_DEFUSEKIT;
         }
         else if (strncmp ("grenade", STRING (ent->v.classname), 7) == 0 && strcmp (STRING (ent->v.model) + 9, "c4.mdl") == 0)
         {
            allowPickup = true;
            pickupType = PICKTYPE_PLANTEDC4;
         }
      }

      if (allowPickup) // if the bot found something it can pickup...
      {
         distance = (entityOrigin - pev->origin).GetLength ();

         // see if it's the closest item so far...
         if (distance < minDistance)
         {
            if (pickupType == PICKTYPE_WEAPON) // found weapon on ground?
            {
               int weaponCarried = GetBestWeaponCarried ();
               int secondaryWeaponCarried = GetBestSecondaryWeaponCarried ();

               if (secondaryWeaponCarried < 7 && (m_ammo[g_weaponSelect[secondaryWeaponCarried].id] > 0.3 * g_weaponDefs[g_weaponSelect[secondaryWeaponCarried].id].ammo1Max) && strcmp (STRING (ent->v.model) + 9, "w_357ammobox.mdl") == 0)
                  allowPickup = false;
               else if (!m_isVIP && weaponCarried >= 7 && (m_ammo[g_weaponSelect[weaponCarried].id] > 0.3 * g_weaponDefs[g_weaponSelect[weaponCarried].id].ammo1Max) && strncmp (STRING (ent->v.model) + 9, "w_", 2) == 0)
               {
                  if (strcmp (STRING (ent->v.model) + 9, "w_9mmarclip.mdl") == 0 && !(weaponCarried == WEAPON_FAMAS || weaponCarried == WEAPON_AK47 || weaponCarried == WEAPON_M4A1 || weaponCarried == WEAPON_GALIL || weaponCarried == WEAPON_AUG || weaponCarried == WEAPON_SG552))
                     allowPickup = false;
                  else if (strcmp (STRING (ent->v.model) + 9, "w_shotbox.mdl") == 0 && !(weaponCarried == WEAPON_M3 || weaponCarried == WEAPON_XM1014))
                     allowPickup = false;
                  else if (strcmp (STRING (ent->v.model) + 9, "w_9mmclip.mdl") == 0 && !(weaponCarried == WEAPON_MP5 || weaponCarried == WEAPON_TMP || weaponCarried == WEAPON_P90 || weaponCarried == WEAPON_MAC10 || weaponCarried == WEAPON_UMP45))
                     allowPickup = false;
                  else if (strcmp (STRING (ent->v.model) + 9, "w_crossbow_clip.mdl") == 0 && !(weaponCarried == WEAPON_AWP || weaponCarried == WEAPON_G3SG1 || weaponCarried == WEAPON_SCOUT || weaponCarried == WEAPON_SG550))
                     allowPickup = false;
                  else if (strcmp (STRING (ent->v.model) + 9, "w_chainammo.mdl") == 0 && weaponCarried == WEAPON_M249)
                     allowPickup = false;
               }
               else if (m_isVIP || !RateGroundWeapon (ent))
                  allowPickup = false;
               else if (strcmp (STRING (ent->v.model) + 9, "medkit.mdl") == 0 && (pev->health >= 100))
                  allowPickup = false;
               else if ((strcmp (STRING (ent->v.model) + 9, "kevlar.mdl") == 0 || strcmp (STRING (ent->v.model) + 9, "battery.mdl") == 0) && pev->armorvalue >= 100) // armor vest
                  allowPickup = false;
               else if (strcmp (STRING (ent->v.model) + 9, "flashbang.mdl") == 0 && (pev->weapons & (1 << WEAPON_FBGRENADE))) // concussion grenade
                  allowPickup = false;
               else if (strcmp (STRING (ent->v.model) + 9, "hegrenade.mdl") == 0 && (pev->weapons & (1 << WEAPON_HEGRENADE))) // explosive grenade
                  allowPickup = false;
               else if (strcmp (STRING (ent->v.model) + 9, "smokegrenade.mdl") == 0 && (pev->weapons & (1 << WEAPON_SMGRENADE))) // smoke grenade
                  allowPickup = false;
            }
            else if (pickupType == PICKTYPE_SHIELDGUN) // found a shield on ground?
            {
               if ((pev->weapons & (1 << WEAPON_ELITE)) || HasShield () || m_isVIP || (HasPrimaryWeapon () && !RateGroundWeapon (ent)))
                  allowPickup = false;
            }
            else if (GetTeam (GetEntity ()) == TEAM_TERRORIST) // terrorist team specific
            {
               if (pickupType == PICKTYPE_DROPPEDC4)
               {
                  allowPickup = true;
                  m_destOrigin = entityOrigin; // ensure we reached droped bomb

                  ChatterMessage (Chatter_FoundC4); // play info about that
               }
               else if (pickupType == PICKTYPE_HOSTAGE)
               {
                  m_itemIgnore = ent;
                  allowPickup = false;

                  if (m_skill > 80 && engine->RandomInt (0, 100) < 50 && GetCurrentTask ()->taskID != TASK_MOVETOPOSITION && GetCurrentTask ()->taskID != TASK_CAMP)
                  {
                     int index = FindDefendWaypoint (entityOrigin);

                     PushTask (TASK_CAMP, TASKPRI_CAMP, -1, engine->GetTime () + engine->RandomFloat (60.0f, 120.0f), true); // push camp task on to stack
                     PushTask (TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, index, engine->GetTime () + engine->RandomFloat (3.0f, 6.0f), true); // push move command

                     if (g_waypoint->GetPath (index)->vis.crouch <= g_waypoint->GetPath (index)->vis.stand)
                        m_campButtons |= IN_DUCK;
                     else
                        m_campButtons &= ~IN_DUCK;

                     ChatterMessage (Chatter_GoingToGuardHostages); // play info about that
                     return;
                  }
               }
               else if (pickupType == PICKTYPE_PLANTEDC4)
               {
                  allowPickup = false;

                  if (!m_defendedBomb)
                  {
                     m_defendedBomb = true;

                     int index = FindDefendWaypoint (entityOrigin);
                     float timeMidBlowup = g_timeBombPlanted + ((engine->GetC4TimerTime () / 2) + engine->GetC4TimerTime () / 4) - g_waypoint->GetTravelTime (pev->maxspeed, pev->origin, g_waypoint->GetPath (index)->origin);

                     if (timeMidBlowup > engine->GetTime ())
                     {
                        RemoveCertainTask (TASK_MOVETOPOSITION); // remove any move tasks

                        PushTask (TASK_CAMP, TASKPRI_CAMP, -1, timeMidBlowup, true); // push camp task on to stack
                        PushTask (TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, index, timeMidBlowup, true); // push move command

                        if (g_waypoint->GetPath (index)->vis.crouch <= g_waypoint->GetPath (index)->vis.stand)
                           m_campButtons |= IN_DUCK;
                        else
                           m_campButtons &= ~IN_DUCK;

                        if (engine->RandomInt (0, 100) < 90)
                           ChatterMessage (Chatter_DefendingBombSite);
                     }
                     else
                        RadioMessage (Radio_ShesGonnaBlow); // issue an additional radio message
                  }
               }
            }
            else if (GetTeam (GetEntity ()) == TEAM_COUNTER)
            {
               if (pickupType == PICKTYPE_HOSTAGE)
               {
                  if (FNullEnt (ent) || (ent->v.health <= 0))
                     allowPickup = false; // never pickup dead hostage
                  else for (int i = 0; i < engine->GetMaxClients (); i++)
                  {
                     if ((bot = g_botManager->GetBot (i)) != null && IsAlive (bot->GetEntity ()))
                     {
                        for (int j = 0; j < Const_MaxHostages; j++)
                        {
                           if (bot->m_hostages[j] == ent)
                           {
                              allowPickup = false;
                              break;
                           }
                        }
                     }
                  }
               }
               else if (pickupType == PICKTYPE_PLANTEDC4)
               {
                  if (m_states & (STATE_SEEINGENEMY | STATE_SUSPECTENEMY) || OutOfBombTimer ())
                  {
                     allowPickup = false;
                     return;
                  }

                  if (engine->RandomInt (0, 100) < 90)
                     ChatterMessage (Chatter_FoundBombPlace);

                  allowPickup = !IsBombDefusing (g_waypoint->GetBombPosition ());
                  pickupType = PICKTYPE_PLANTEDC4;

                  if (!m_defendedBomb && !allowPickup)
                  {
                     m_defendedBomb = true;

                     int index = FindDefendWaypoint (entityOrigin);
                     float timeBlowup = g_timeBombPlanted + engine->GetC4TimerTime () - g_waypoint->GetTravelTime (pev->maxspeed, pev->origin, g_waypoint->GetPath (index)->origin);

                     RemoveCertainTask (TASK_MOVETOPOSITION); // remove any move tasks

                     PushTask (TASK_CAMP, TASKPRI_CAMP, -1, timeBlowup, true); // push camp task on to stack
                     PushTask (TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, index, timeBlowup, true); // push move command

                     if (g_waypoint->GetPath (index)->vis.crouch <= g_waypoint->GetPath (index)->vis.stand)
                        m_campButtons |= IN_DUCK;
                     else
                        m_campButtons &= ~IN_DUCK;

                     if (engine->RandomInt (0, 100) < 90)
                        ChatterMessage (Chatter_DefendingBombSite);
                  }
               }
               else if (pickupType == PICKTYPE_DROPPEDC4)
               {
                  m_itemIgnore = ent;
                  allowPickup = false;

                  if (m_skill > 80 && engine->RandomInt (0, 100) < 90)
                  {
                     int index = FindDefendWaypoint (entityOrigin);

                     PushTask (TASK_CAMP, TASKPRI_CAMP, -1, engine->GetTime () + engine->RandomFloat (60.0, 120.0f), true); // push camp task on to stack
                     PushTask (TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, index, engine->GetTime () + engine->RandomFloat (10.0, 30.0f), true); // push move command

                     if (g_waypoint->GetPath (index)->vis.crouch <= g_waypoint->GetPath (index)->vis.stand)
                        m_campButtons |= IN_DUCK;
                     else
                        m_campButtons &= ~IN_DUCK;

                     ChatterMessage (Chatter_GoingToGuardDoppedBomb); // play info about that
                     return;
                  }
               }
            }

            // if condition valid
            if (allowPickup)
            {
               minDistance = distance; // update the minimum distance
               pickupOrigin = entityOrigin; // remember location of entity
               pickupItem = ent; // remember this entity

               m_pickupType = pickupType;
            }
            else
            {
               pickupItem = null;
               pickupType = PICKTYPE_NONE;
            }
         }
      }
   } // end of the while loop

   if (!FNullEnt (pickupItem))
   {
      for (int i = 0; i < engine->GetMaxClients (); i++)
      {
         if ((bot = g_botManager->GetBot (i)) != null && IsAlive (bot->GetEntity ()) && bot->m_pickupItem == pickupItem)
         {
            m_pickupItem = null;
            m_pickupType = PICKTYPE_NONE;

            return;
         }
      }

      if (pickupOrigin.z > EyePosition ().z + 12.0f  || IsDeadlyDrop (pickupOrigin)) // check if item is too high to reach, check if getting the item would hurt bot
      {
         m_pickupItem = null;
         m_pickupType = PICKTYPE_NONE;

         return;
      }
      m_pickupItem = pickupItem; // save pointer of picking up entity
   }
}

void Bot::GetCampDirection (Vector *dest)
{
   // this function check if view on last enemy position is blocked - replace with better vector then
   // mostly used for getting a good camping direction vector if not camping on a camp waypoint

   TraceResult tr;
   Vector src = EyePosition ();

   TraceLine (src, *dest, true, GetEntity (), &tr);

   // check if the trace hit something...
   if (tr.flFraction < 1.0f)
   {
      float length = (tr.vecEndPos - src).GetLengthSquared ();

      if (length > 10000)
         return;

      float minDistance = FLT_MAX;
      float maxDistance = FLT_MAX;

      int enemyIndex = -1, tempIndex = -1;

      // find nearest waypoint to bot and position
      for (int i = 0; i < g_numWaypoints; i++)
      {
         float distance = (g_waypoint->GetPath (i)->origin - pev->origin).GetLengthSquared ();

         if (distance < minDistance)
         {
            minDistance = distance;
            tempIndex = i;
         }
         distance = (g_waypoint->GetPath (i)->origin - *dest).GetLengthSquared ();

         if (distance < maxDistance)
         {
            maxDistance = distance;
            enemyIndex = i;
         }
      }

      if (tempIndex == -1 || enemyIndex == -1)
         return;

      minDistance = FLT_MAX;

      int lookAtWaypoint = -1;
      Path *path = g_waypoint->GetPath (tempIndex);

      for (int i = 0; i < Const_MaxPathIndex; i++)
      {
         if (path->index[i] == -1)
            continue;

         float distance = g_waypoint->GetPathDistanceFloat (path->index[i], enemyIndex);

         if (distance < minDistance)
         {
            minDistance = distance;
            lookAtWaypoint = path->index[i];
         }
      }
      if (lookAtWaypoint != -1 && lookAtWaypoint < g_numWaypoints)
         *dest = g_waypoint->GetPath (lookAtWaypoint)->origin;
   }
}

void Bot::SwitchChatterIcon (bool show)
{
   // this function depending on show boolen, shows/remove chatter, icon, on the head of bot.

   if (g_gameVersion == CSVER_VERYOLD)
      return;

   for (int i = 0; i < engine->GetMaxClients (); i++)
   {
      edict_t *ent = INDEXENT (i);

      if (!IsValidPlayer (ent) || IsValidBot (ent) || GetTeam (ent) != GetTeam (GetEntity ()))
         continue;

      MESSAGE_BEGIN (MSG_ONE, g_netMsg->GetId (NETMSG_BOTVOICE), null, ent); // begin message
         WRITE_BYTE (show); // switch on/off
         WRITE_BYTE (GetIndex ());
      MESSAGE_END ();

   }
}

void Bot::InstantChatterMessage (int type)
{
   // this function sends instant chatter messages.

   if (yb_commtype.GetInt () != 2 || g_chatterFactory[type].IsEmpty () || g_gameVersion == CSVER_VERYOLD || !g_sendAudioFinished)
      return;

   if (IsAlive (GetEntity ()))
      SwitchChatterIcon (true);

   static float reportTime = engine->GetTime ();

   // delay only reportteam
   if (type == Radio_ReportTeam)
   {
      if (reportTime >= engine->GetTime ())
         return;

      reportTime = engine->GetTime () + engine->RandomFloat (30.0, 80.0f);
   }

   String defaultSound = g_chatterFactory[type].GetRandomElement ().name;
   String painSound = g_chatterFactory[Chatter_DiePain].GetRandomElement ().name;

   for (int i = 0; i < engine->GetMaxClients (); i++)
   {
      edict_t *ent = INDEXENT (i);

      if (!IsValidPlayer (ent) || IsValidBot (ent) || GetTeam (ent) != GetTeam (GetEntity ()))
         continue;

      g_sendAudioFinished = false;

      MESSAGE_BEGIN (MSG_ONE, g_netMsg->GetId (NETMSG_SENDAUDIO), null, ent); // begin message
         WRITE_BYTE (GetIndex ());

         if (pev->deadflag & DEAD_DYING)
            WRITE_STRING (FormatBuffer ("%s/%s.wav", yb_chatterpath.GetString (), (const char*)painSound));
         else if (!(pev->deadflag & DEAD_DEAD))
            WRITE_STRING (FormatBuffer ("%s/%s.wav", yb_chatterpath.GetString (), (const char*)defaultSound));

         WRITE_SHORT (m_voicePitch);
      MESSAGE_END ();

      g_sendAudioFinished = true;
   }
}

void Bot::RadioMessage (int message)
{
   // this function inserts the radio message into the message queue

   if (yb_commtype.GetInt () == 0 || GetNearbyFriendsNearPosition (pev->origin, 9999) == 0)
      return;

   if (g_chatterFactory[message].IsEmpty () || g_gameVersion == CSVER_VERYOLD || yb_commtype.GetInt () != 2)
      g_radioInsteadVoice = true; // use radio instead voice

   m_radioSelect = message;
   PushMessageQueue (CMENU_RADIO);
}

void Bot::ChatterMessage (int message)
{
   // this function inserts the voice message into the message queue (mostly same as above)

   if (yb_commtype.GetInt () != 2 || g_chatterFactory[message].IsEmpty () || GetNearbyFriendsNearPosition (pev->origin, 9999) == 0)
      return;

   bool shouldExecute = false;

   if (m_voiceTimers[message] < engine->GetTime () || m_voiceTimers[message] == FLT_MAX)
   {
      if (m_voiceTimers[message] != FLT_MAX)
         m_voiceTimers[message] = engine->GetTime () + g_chatterFactory[message][0].repeatTime;

      shouldExecute = true;
   }

   if (!shouldExecute)
      return;

   m_radioSelect = message;
   PushMessageQueue (CMENU_RADIO);
}

void Bot::CheckMessageQueue (void)
{
   // this function checks and executes pending messages

   // no new message?
   if (m_actMessageIndex == m_pushMessageIndex)
      return;

   // get message from stack
   int currentQueueMessage = GetMessageQueue ();

   // nothing to do?
   if (currentQueueMessage == CMENU_IDLE || (currentQueueMessage == CMENU_RADIO && yb_csdmplay.GetInt () == 2))
      return;

   int team = GetTeam (GetEntity ());

   switch (currentQueueMessage)
   {
   case CMENU_BUY: // general buy message

      // buy weapon
      if (m_nextBuyTime > engine->GetTime ())
      {
         // keep sending message
         PushMessageQueue (CMENU_BUY);
         return;
      }

      if (!m_inBuyZone || yb_csdmplay.GetBool ())
      {
         m_buyPending = true;
         m_buyingFinished = true;

         break;
      }

      m_buyPending = false;
      m_nextBuyTime = engine->GetTime () + engine->RandomFloat (0.3f, 0.8f);

      // if bot buying is off then no need to buy
      if (!yb_botbuy.GetBool ())
         m_buyState = 6;

      // if fun-mode no need to buy
      if (yb_knifemode.GetBool ())
      {
         m_buyState = 6;
         SelectWeaponByName ("weapon_knife");
      }

      // prevent vip from buying
      if (g_mapType & MAP_AS)
      {
         if (*(INFOKEY_VALUE (GET_INFOKEYBUFFER (GetEntity ()), "model")) == 'v')
         {
            m_isVIP = true;
            m_buyState = 6;
            m_pathType = 1;
         }
      }

      // prevent terrorists from buying on es maps
      if ((g_mapType & MAP_ES) && GetTeam (GetEntity ()) == TEAM_TERRORIST)
         m_buyState = 6;

      // prevent teams from buying on fun maps
      if (g_mapType & (MAP_KA | MAP_FY))
      {
         m_buyState = 6;

         if (g_mapType & MAP_KA)
            yb_knifemode.SetInt (1);
      }

      if (m_buyState > 5)
      {
         m_buyingFinished = true;
         return;
      }

      PushMessageQueue (CMENU_IDLE);
      PerformWeaponPurchase ();

      break;

   case CMENU_RADIO: // general radio message issued
     // if last bot radio command (global) happened just a second ago, delay response
      if (g_lastRadioTime[team] + 1.0f < engine->GetTime ())
      {
         // if same message like previous just do a yes/no
         if (m_radioSelect != Radio_Affirmative && m_radioSelect != Radio_Negative)
         {
            if (m_radioSelect == g_lastRadio[team] && g_lastRadioTime[team] + 1.5 > engine->GetTime ())
               m_radioSelect = -1;
            else
            {
               if (m_radioSelect != Radio_ReportingIn)
                  g_lastRadio[team] = m_radioSelect;
               else
                  g_lastRadio[team] = -1;

               for (int i = 0; i < engine->GetMaxClients (); i++)
               {
                  if (g_botManager->GetBot (i))
                  {
                     if (pev != g_botManager->GetBot (i)->pev && GetTeam (g_botManager->GetBot (i)->GetEntity ()) == team)
                     {
                        g_botManager->GetBot (i)->m_radioOrder = m_radioSelect;
                        g_botManager->GetBot (i)->m_radioEntity = GetEntity ();
                     }
                  }
               }
            }
         }

         if (m_radioSelect == Radio_ReportingIn)
         {
            switch (GetCurrentTask ()->taskID)
            {
            case TASK_NORMAL:
               if (GetCurrentTask ()->data != -1)
               {
                  if (g_waypoint->GetPath (GetCurrentTask ()->data)->flags & WAYPOINT_GOAL)
                  {
                     if ((g_mapType & MAP_DE) && GetTeam (GetEntity ()) == TEAM_TERRORIST && (pev->weapons & (1 << WEAPON_C4)))
                        InstantChatterMessage (Chatter_GoingToPlantBomb);
                     else
                        InstantChatterMessage (Chatter_Nothing);
                  }
                  else if (g_waypoint->GetPath (GetCurrentTask ()->data)->flags & WAYPOINT_RESCUE)
                     InstantChatterMessage (Chatter_RescuingHostages);
                  else if (g_waypoint->GetPath (GetCurrentTask ()->data)->flags & WAYPOINT_CAMP)
                     InstantChatterMessage (Chatter_GoingToCamp);
                  else
                     InstantChatterMessage (Chatter_HearSomething);
               }
               else
                  InstantChatterMessage (Chatter_ReportingIn);
               break;

            case TASK_MOVETOPOSITION:
               InstantChatterMessage (Chatter_GoingToCamp);
               break;

            case TASK_CAMP:
               if (g_bombPlanted && GetTeam (GetEntity ()) == TEAM_TERRORIST)
                  InstantChatterMessage (Chatter_GuardDroppedC4);
               else if (m_inVIPZone && GetTeam (GetEntity ()) == TEAM_TERRORIST)
                  InstantChatterMessage (Chatter_GuardingVipSafety);
               else
                  InstantChatterMessage (Chatter_Camp);
               break;

            case TASK_PLANTBOMB:
               InstantChatterMessage (Chatter_PlantingC4);
               break;

            case TASK_DEFUSEBOMB:
               InstantChatterMessage (Chatter_DefusingC4);
               break;

            case TASK_FIGHTENEMY:
               InstantChatterMessage (Chatter_InCombat);
               break;

            case TASK_HIDE:
            case TASK_SEEKCOVER:
               InstantChatterMessage (Chatter_SeeksEnemy);
               break;

            default:
               InstantChatterMessage (Chatter_Nothing);
               break;
            }
         }

         if (m_radioSelect != Radio_ReportingIn && g_radioInsteadVoice || yb_commtype.GetInt () != 2 || g_chatterFactory[m_radioSelect].IsEmpty () || g_gameVersion == CSVER_VERYOLD)
         {
            if (m_radioSelect < Radio_GoGoGo)
               FakeClientCommand (GetEntity (), "radio1");
            else if (m_radioSelect < Radio_Affirmative)
            {
               m_radioSelect -= Radio_GoGoGo - 1;
               FakeClientCommand (GetEntity (), "radio2");
            }
            else
            {
               m_radioSelect -= Radio_Affirmative - 1;
               FakeClientCommand (GetEntity (), "radio3");
            }

            // select correct menu item for this radio message
            FakeClientCommand (GetEntity (), "menuselect %d", m_radioSelect);
         }
         else if (m_radioSelect != -1 && m_radioSelect != Radio_ReportingIn)
            InstantChatterMessage (m_radioSelect);

         g_radioInsteadVoice = false; // reset radio to voice
         g_lastRadioTime[team] = engine->GetTime (); // store last radio usage
      }
      else
         PushMessageQueue (CMENU_RADIO);
      break;

   // team independent saytext
   case CMENU_SAY:
      SayText (m_tempStrings);
      break;

   // team dependent saytext
   case CMENU_TEAMSAY:
      TeamSayText (m_tempStrings);
      break;

   case CMENU_IDLE:
   default:
      return;
   }
}

bool Bot::IsRestricted (int weaponIndex)
{
   // this function checks for weapon restrictions.

   if (IsNullString (yb_restrictweapons.GetString ()))
      return false; // no banned weapons

   Array <String> bannedWeapons = String (yb_restrictweapons.GetString ()).Split (";");

   ITERATE_ARRAY (bannedWeapons, i)
   {
      const char *banned = STRING (GetWeaponReturn (true, null, weaponIndex));

      // check is this weapon is banned
      if (strncmp (bannedWeapons[i], banned, bannedWeapons[i].GetLength ()) == 0)
         return true;
   }

   if (m_buyingFinished)
      return false;

   return IsRestrictedAMX (weaponIndex);
}

bool Bot::IsRestrictedAMX (int weaponIndex)
{
   // this function checks restriction set by AMX Mod, this function code is courtesy of KWo.

   const char *restrictedWeapons = CVAR_GET_STRING ("amx_restrweapons");
   const char *restrictedEquipment = CVAR_GET_STRING ("amx_restrequipammo");

   // check for weapon restrictions
   if ((1 << weaponIndex) & (WeaponBits_Primary | WeaponBits_Secondary | WEAPON_SHIELDGUN))
   {
      if (IsNullString (restrictedWeapons))
         return false;

      int indices[] = {4, 25, 20, -1, 8, -1, 12, 19, -1, 5, 6, 13, 23, 17, 18, 1, 2, 21, 9, 24, 7, 16, 10, 22, -1, 3, 15, 14, 0, 11};

      // find the weapon index
      int index = indices[weaponIndex - 1];

      // validate index range
      if (index < 0 || index >= static_cast <int> (strlen (restrictedWeapons)))
         return false;

      return restrictedWeapons[index] != '0';
   }
   else // check for equipment restrictions
   {
      if (IsNullString (restrictedEquipment))
         return false;

      int indices[] = {-1, -1, -1, 3, -1, -1, -1, -1, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2, -1, -1, -1, -1, -1, 0, 1, 5};

      // find the weapon index
      int index = indices[weaponIndex - 1];

      // validate index range
      if (index < 0 || index >= static_cast <int> (strlen (restrictedEquipment)))
         return false;

      return restrictedEquipment[index] != '0';
   }
}

bool Bot::IsMorePowerfulWeaponCanBeBought (void)
{
   // this function determines currently owned primary weapon, and checks if bot has
   // enough money to buy more powerful weapon.

   // if bot is not rich enough or non-standard weapon mode enabled return false
   if (g_weaponSelect[25].teamStandard != 1 || m_moneyAmount < 4000 || IsNullString (yb_restrictweapons.GetString ()))
      return false;

   // also check if bot has really bad weapon, maybe it's time to change it
   if (UsesBadPrimary ())
      return true;

   Array <String> bannedWeapons = String (yb_restrictweapons.GetString ()).Split (";");

   // check if its banned
   ITERATE_ARRAY (bannedWeapons, i)
   {
      if (m_currentWeapon == GetWeaponReturn (false, bannedWeapons[i]))
         return true;
   }

   if (m_currentWeapon == WEAPON_SCOUT && m_moneyAmount > 5000)
      return true;
   else if (m_currentWeapon == WEAPON_MP5 && m_moneyAmount > 6000)
      return true;
   else if (m_currentWeapon == WEAPON_M3 || m_currentWeapon == WEAPON_XM1014 && m_moneyAmount > 4000)
      return true;

   return false;
}

void Bot::PerformWeaponPurchase (void)
{
   // this function does all the work in selecting correct buy menus for most weapons/items

   WeaponSelect *selectedWeapon = null;
   m_nextBuyTime = engine->GetTime ();

   int count = 0, foundWeapons = 0, economicsPrice = 0;
   int choices[Const_NumWeapons];

   // select the priority tab for this personality
   int *ptr = g_weaponPrefs[m_personality] + Const_NumWeapons;
   int team = GetTeam (GetEntity ());

   bool isPistolMode = g_weaponSelect[25].teamStandard == -1 && g_weaponSelect[3].teamStandard == 2;

   switch (m_buyState)
   {
   case 0: // if no primary weapon and bot has some money, buy a primary weapon
      if ((!HasShield () && !HasPrimaryWeapon ()) && (g_botManager->EconomicsValid (team) || IsMorePowerfulWeaponCanBeBought ()))
      {
         do
         {
            bool ignoreWeapon = false;

            ptr--;

            InternalAssert (*ptr > -1);
            InternalAssert (*ptr < Const_NumWeapons);

            selectedWeapon = &g_weaponSelect[*ptr];
            count++;

            if (selectedWeapon->buyGroup == 1)
               continue;

            // weapon available for every team?
            if ((g_mapType & MAP_AS) && selectedWeapon->teamAS != 2 && selectedWeapon->teamAS != team)
               continue;

            // ignore weapon if this weapon not supported by currently running cs version...
            if (g_gameVersion == CSVER_VERYOLD && selectedWeapon->buySelect == -1)
               continue;

            // ignore weapon if this weapon is not targeted to out team....
            if (selectedWeapon->teamStandard != 2 && selectedWeapon->teamStandard != team)
               continue;

            // ignore weapon if this weapon is restricted
            if (IsRestricted (selectedWeapon->id))
               continue;

            // filter out weapons with bot economics (thanks for idea to nicebot project)
            switch (m_personality)
            {
            case PERSONALITY_RUSHER:
               economicsPrice = g_botBuyEconomyTable[8];
               break;

            case PERSONALITY_CAREFUL:
               economicsPrice = g_botBuyEconomyTable[7];
               break;

            case PERSONALITY_NORMAL:
               economicsPrice = g_botBuyEconomyTable[9];
               break;
            }

            if (team == TEAM_COUNTER)
            {
               switch (selectedWeapon->id)
               {
               case WEAPON_TMP:
               case WEAPON_UMP45:
               case WEAPON_P90:
               case WEAPON_MP5:
                  if (m_moneyAmount > g_botBuyEconomyTable[1] + economicsPrice)
                     ignoreWeapon = true;
                  break;
               }

               if (selectedWeapon->id == WEAPON_SHIELDGUN && m_moneyAmount > g_botBuyEconomyTable[10])
                  ignoreWeapon = true;
            }
            else if (team == TEAM_TERRORIST)
            {
               switch (selectedWeapon->id)
               {
               case WEAPON_UMP45:
               case WEAPON_MAC10:
               case WEAPON_P90:
               case WEAPON_MP5:
               case WEAPON_SCOUT:
                  if (m_moneyAmount > g_botBuyEconomyTable[2] + economicsPrice)
                     ignoreWeapon = true;
                  break;
               }
            }

            switch (selectedWeapon->id)
            {
            case WEAPON_XM1014:
            case WEAPON_M3:
               if (m_moneyAmount < g_botBuyEconomyTable[3] || m_moneyAmount > g_botBuyEconomyTable[4])
                  ignoreWeapon = true;
               break;
            }

            switch (selectedWeapon->id)
            {
            case WEAPON_SG550:
            case WEAPON_G3SG1:
            case WEAPON_AWP:
            case WEAPON_M249:
               if (m_moneyAmount < g_botBuyEconomyTable[5] || m_moneyAmount > g_botBuyEconomyTable[6])
                  ignoreWeapon = true;
               break;
            }

            if (ignoreWeapon && g_weaponSelect[25].teamStandard == 1 && yb_ecorounds.GetBool ())
               continue;

            int moneySave = engine->RandomInt (100, 300);

            if (g_botManager->GetLastWinner () == team)
               moneySave = 0;

            if (selectedWeapon->price < m_moneyAmount - moneySave)
               choices[foundWeapons++] = *ptr;

         } while (count < Const_NumWeapons && foundWeapons < 4);

         // found a desired weapon?
         if (foundWeapons > 0)
         {
            int chosenWeapon = choices[foundWeapons - 1];

            // choose randomly from the best ones...
            if (foundWeapons > 1)
            {
               if (yb_hardmode.GetBool () && engine->RandomInt (0, 100) < engine->RandomInt (50, 60) + m_skill * 0.3)
                  chosenWeapon = choices[0];
               else
                  chosenWeapon = choices[engine->RandomInt (engine->RandomInt (0, foundWeapons - 1), engine->RandomInt (0, foundWeapons - 1))];
            }
            selectedWeapon = &g_weaponSelect[chosenWeapon];
         }
         else
            selectedWeapon = null;

         if (selectedWeapon != null)
         {
            FakeClientCommand (GetEntity (), "buy;menuselect %d", selectedWeapon->buyGroup);

            if (g_gameVersion == CSVER_VERYOLD)
               FakeClientCommand (GetEntity (), "menuselect %d", selectedWeapon->buySelect);
            else // SteamCS buy menu is different from the old one
            {
               if (GetTeam (GetEntity ()) == TEAM_TERRORIST)
                  FakeClientCommand (GetEntity (), "menuselect %d", selectedWeapon->newBuySelectT);
               else
                  FakeClientCommand (GetEntity (), "menuselect %d", selectedWeapon->newBuySelectCT);
            }
         }
      }
      else if (HasPrimaryWeapon () && !HasShield ())
         m_reloadState = RSTATE_PRIMARY;

      break;

   case 1: // if armor is damaged and bot has some money, buy some armor
      if (pev->armorvalue < engine->RandomInt (50, 80) && (isPistolMode || (g_botManager->EconomicsValid (team) && HasPrimaryWeapon ())))
      {
         // if bot is rich, buy kevlar + helmet, else buy a single kevlar
         if (m_moneyAmount > 1500 && !IsRestricted (WEAPON_KEVHELM))
            FakeClientCommand (GetEntity (), "buyequip;menuselect 2");
         else if (!IsRestricted (WEAPON_KEVLAR))
            FakeClientCommand (GetEntity (), "buyequip;menuselect 1");
      }
      break;

   case 2: // if bot has still some money, buy a better secondary weapon
      if (isPistolMode || (HasPrimaryWeapon () && (pev->weapons & ((1 << WEAPON_USP) | (1 << WEAPON_GLOCK18))) && m_moneyAmount > engine->RandomInt (7500, 9000)))
      {
         do
         {
            ptr--;

            InternalAssert (*ptr > -1);
            InternalAssert (*ptr < Const_NumWeapons);

            selectedWeapon = &g_weaponSelect[*ptr];
            count++;

            if (selectedWeapon->buyGroup != 1)
               continue;

            // ignore weapon if this weapon is restricted
            if (IsRestricted (selectedWeapon->id))
               continue;

            // weapon available for every team?
            if ((g_mapType & MAP_AS) && selectedWeapon->teamAS != 2 && selectedWeapon->teamAS != team)
               continue;

            if (g_gameVersion == CSVER_VERYOLD && selectedWeapon->buySelect == -1)
               continue;

            if (selectedWeapon->teamStandard != 2 && selectedWeapon->teamStandard != team)
               continue;

            if (selectedWeapon->price <= (m_moneyAmount - engine->RandomInt (100, 200)))
               choices[foundWeapons++] = *ptr;

         } while (count < Const_NumWeapons && foundWeapons < 4);

         // found a desired weapon?
         if (foundWeapons > 0)
         {
            int chosenWeapon = choices[foundWeapons - 1];

            // choose randomly from the best ones...
            if (foundWeapons > 1)
            {
               if (yb_hardmode.GetBool () && engine->RandomInt (0, 100) < engine->RandomInt (50, 60) + m_skill * 0.3)
                  chosenWeapon = choices[0];
               else
                  chosenWeapon = choices[engine->RandomInt (engine->RandomInt (0, foundWeapons - 1), engine->RandomInt (0, foundWeapons - 1))];
            }
            selectedWeapon = &g_weaponSelect[chosenWeapon];
         }
         else
            selectedWeapon = null;

         // found a desired weapon?
         if (foundWeapons > 0)
         {
            int chosenWeapon;

            // choose randomly from the best ones...
            if (foundWeapons > 1)
               chosenWeapon = choices[engine->RandomInt (0, foundWeapons - 1)];
            else
               chosenWeapon = choices[foundWeapons - 1];

            selectedWeapon = &g_weaponSelect[chosenWeapon];
         }
         else
            selectedWeapon = null;

         if (selectedWeapon != null)
         {
            FakeClientCommand (GetEntity (), "buy;menuselect %d", selectedWeapon->buyGroup);

            if (g_gameVersion == CSVER_VERYOLD)
               FakeClientCommand (GetEntity (), "menuselect %d", selectedWeapon->buySelect);
            else // SteamCS buy menu is different from old one
            {
               if (GetTeam (GetEntity ()) == TEAM_TERRORIST)
                  FakeClientCommand (GetEntity (), "menuselect %d", selectedWeapon->newBuySelectT);
               else
                  FakeClientCommand (GetEntity (), "menuselect %d", selectedWeapon->newBuySelectCT);
            }
         }
      }
      break;

   case 3: // if bot has still some money, choose if bot should buy a grenade or not
      if (engine->RandomInt (1, 100) < g_grenadeBuyPrecent[0] && m_moneyAmount >= 300 && !IsRestricted (WEAPON_HEGRENADE))
      {
         // buy a he grenade
         FakeClientCommand (GetEntity (), "buyequip");
         FakeClientCommand (GetEntity (), "menuselect 4");
      }

      if (engine->RandomInt (1, 100) < g_grenadeBuyPrecent[1] && m_moneyAmount >= 200 && g_botManager->EconomicsValid (team) && !IsRestricted (WEAPON_FBGRENADE))
      {
         // buy a concussion grenade, i.e., 'flashbang'
         FakeClientCommand (GetEntity (), "buyequip");
         FakeClientCommand (GetEntity (), "menuselect 3");
      }

      if (engine->RandomInt (1, 100) < g_grenadeBuyPrecent[2] && m_moneyAmount >= 300 && g_botManager->EconomicsValid (team) && !IsRestricted (WEAPON_SMGRENADE))
      {
         // buy a smoke grenade
         FakeClientCommand (GetEntity (), "buyequip");
         FakeClientCommand (GetEntity (), "menuselect 5");
      }
      break;

   case 4: // if bot is CT and we're on a bomb map, randomly buy the defuse kit
      if ((g_mapType & MAP_DE) && GetTeam (GetEntity ()) == TEAM_COUNTER && engine->RandomInt (1, 100) < 80 && m_moneyAmount > 200 && !IsRestricted (WEAPON_DEFUSER))
      {
         if (g_gameVersion == CSVER_VERYOLD)
            FakeClientCommand (GetEntity (), "buyequip;menuselect 6");
         else
            FakeClientCommand (GetEntity (), "defuser"); // use alias in SteamCS
      }
      break;

   case 5: // buy enough primary & secondary ammo (do not check for money here)
      for (int i = 0; i <= 5; i++)
         FakeClientCommand (GetEntity (), "buyammo%d", engine->RandomInt (1, 2)); // simulate human

      // buy enough secondary ammo
      if (HasPrimaryWeapon ())
         FakeClientCommand (GetEntity (), "buy;menuselect 7");

      // buy enough primary ammo
      FakeClientCommand (GetEntity (), "buy;menuselect 6");

      // try to reload secondary weapon
      if (m_reloadState != RSTATE_PRIMARY)
         m_reloadState = RSTATE_SECONDARY;

      break;
   }

   m_buyState++;
   PushMessageQueue (CMENU_BUY);
}

Task *ClampDesire (Task *first, float min, float max)
{
   // this function iven some values min and max, clamp the inputs to be inside the [min, max] range.

   if (first->desire < min)
      first->desire = min;
   else if (first->desire > max)
      first->desire = max;

   return first;
}

Task *MaxDesire (Task *first, Task *second)
{
   // this function returns the behavior having the higher activation level.

   if (first->desire > second->desire)
      return first;

   return second;
}

Task *SubsumeDesire (Task *first, Task *second)
{
   // this function returns the first behavior if its activation level is anything higher than zero.

   if (first->desire > 0)
      return first;

   return second;
}

Task *ThresholdDesire (Task *first, float threshold, float desire)
{
   // this function returns the input behavior if it's activation level exceeds the threshold, or some default
   // behavior otherwise.

   if (first->desire < threshold)
      first->desire = desire;

   return first;
}

float HysteresisDesire (float cur, float min, float max, float old)
{
   // this function clamp the inputs to be the last known value outside the [min, max] range.

   if (cur <= min || cur >= max)
      old = cur;

   return old;
}

void Bot::SetConditions (void)
{
   // this function carried out each frame. does all of the sensing, calculates emotions and finally sets the desired
   // action after applying all of the Filters

   int team = GetTeam (GetEntity ());
   m_aimFlags = 0;

   // slowly increase/decrease dynamic emotions back to their base level
   if (m_nextEmotionUpdate < engine->GetTime ())
   {
      if (yb_hardmode.GetBool ())
      {
         m_agressionLevel *= 2;
         m_fearLevel *= 0.5f;
      }

      if (m_agressionLevel > m_baseAgressionLevel)
         m_agressionLevel -= 0.05f;
      else
         m_agressionLevel += 0.05f;

      if (m_fearLevel > m_baseFearLevel)
         m_fearLevel -= 0.05f;
      else
         m_fearLevel += 0.05f;

      if (m_agressionLevel < 0.0f)
         m_agressionLevel = 0.0f;

      if (m_fearLevel < 0.0f)
         m_fearLevel = 0.0f;

      m_nextEmotionUpdate = engine->GetTime () + 0.5f;
   }

   // does bot see an enemy?
   if (LookupEnemy ())
      m_states |= STATE_SEEINGENEMY;
   else
   {
      m_states &= ~STATE_SEEINGENEMY;
      m_enemy = null;
   }

   // did bot just kill an enemy?
   if (!FNullEnt (m_lastVictim))
   {
      if (GetTeam (m_lastVictim) != team)
      {
         // add some aggression because we just killed somebody
         m_agressionLevel += 0.1f;

         if (m_agressionLevel > 1.0f)
            m_agressionLevel = 1.0f;

         if (engine->RandomInt (1, 100) > 50)
            ChatMessage (CHAT_KILL);

         if (engine->RandomInt (1, 100) < 10)
            RadioMessage (Radio_EnemyDown);
         else
         {
            if ((m_lastVictim->v.weapons & (1 << WEAPON_AWP)) || (m_lastVictim->v.weapons & (1 << WEAPON_SCOUT)) ||  (m_lastVictim->v.weapons & (1 << WEAPON_G3SG1)) || (m_lastVictim->v.weapons & (1 << WEAPON_SG550)))
               ChatterMessage (Chatter_SniperKilled);
            else
            {
               switch (GetNearbyEnemiesNearPosition (pev->origin, 9999))
               {
               case 0:
                  if (engine->RandomInt (0, 100) < 50)
                     ChatterMessage (Chatter_NoEnemiesLeft);
                  else
                     ChatterMessage (Chatter_EnemyDown);
                  break;

               case 1:
                  ChatterMessage (Chatter_OneEnemyLeft);
                  break;

               case 2:
                  ChatterMessage (Chatter_TwoEnemiesLeft);
                  break;

               case 3:
                  ChatterMessage (Chatter_ThreeEnemiesLeft);
                  break;

               default:
                  ChatterMessage (Chatter_EnemyDown);
               }
            }
         }

         // if no more enemies found AND bomb planted, switch to knife to get to bombplace faster
         if (GetTeam (GetEntity ()) == TEAM_COUNTER && m_currentWeapon != WEAPON_KNIFE && GetNearbyEnemiesNearPosition (pev->origin, 9999) == 0 && g_bombPlanted)
         {
            SelectWeaponByName ("weapon_knife");

            // order team to regroup
            RadioMessage (Radio_RegroupTeam);
         }
      }
      else
      {
         ChatMessage (CHAT_TEAMKILL, true);
         ChatterMessage (Chatter_TeamKill);
      }
      m_lastVictim = null;
   }

   // check if our current enemy is still valid
   if (!FNullEnt (m_lastEnemy))
   {
      if (!IsAlive (m_lastEnemy) && m_shootAtDeadTime < engine->GetTime ())
      {
         m_lastEnemyOrigin = nullvec;
         m_lastEnemy = null;
      }
   }
   else
   {
      m_lastEnemyOrigin = nullvec;
      m_lastEnemy = null;
   }
   extern ConVar yb_noshots;

   // don't listen if seeing enemy, just checked for sounds or being blinded (because its inhuman)
   if (!yb_noshots.GetBool () && m_soundUpdateTime <= engine->GetTime () && m_blindTime < engine->GetTime ())
   {
      ReactOnSound ();
      m_soundUpdateTime = engine->GetTime () + yb_timersound.GetFloat ();
   }
   else if (m_heardSoundTime < engine->GetTime ())
      m_states &= ~STATE_HEARENEMY;

   if (FNullEnt (m_enemy) && !FNullEnt (m_lastEnemy) && m_lastEnemyOrigin != nullvec)
   {
      TraceResult tr;
      TraceLine (EyePosition (), m_lastEnemyOrigin, true, GetEntity (), &tr);

      if ((pev->origin - m_lastEnemyOrigin).GetLength () < 1600.0f && (tr.flFraction >= 0.2 || tr.pHit != g_worldEdict))
      {
         m_aimFlags |= AIM_PREDICTENEMY;

         if (EntityIsVisible (m_lastEnemyOrigin))
            m_aimFlags |= AIM_LASTENEMY;
      }
   }

   // check if throwing a grenade is a good thing to do...
   if (!yb_noshots.GetBool () && !yb_knifemode.GetBool () && m_grenadeCheckTime < engine->GetTime () && !m_isUsingGrenade && GetCurrentTask ()->taskID != TASK_PLANTBOMB && GetCurrentTask ()->taskID != TASK_DEFUSEBOMB && !m_isReloading && IsAlive (m_lastEnemy))
   {
      // check again in some seconds
      m_grenadeCheckTime = engine->GetTime () + yb_timergrenade.GetFloat ();

      // check if we have grenades to throw
      int grenadeToThrow = CheckGrenades ();

      // if we don't have grenades no need to check it this round again
      if (grenadeToThrow == -1)
      {
         m_grenadeCheckTime = engine->GetTime () + 30.0f; // changed since, conzero can drop grens from dead players
         m_states &= ~(STATE_THROWEXPLODE | STATE_THROWFLASH | STATE_THROWSMOKE);
      }
      else
      {
         // care about different types of grenades
         if ((grenadeToThrow == WEAPON_HEGRENADE || grenadeToThrow == WEAPON_SMGRENADE) && engine->RandomInt (0, 100) < 45 && !(m_states & (STATE_SEEINGENEMY | STATE_THROWEXPLODE | STATE_THROWFLASH | STATE_THROWSMOKE)))
         {
            float distance = (m_lastEnemy->v.origin - pev->origin).GetLength ();

            // is enemy to high to throw
            if ((m_lastEnemy->v.origin.z > (pev->origin.z + 500.0f)) || !(m_lastEnemy->v.flags & FL_ONGROUND))
               distance = FLT_MAX; // just some crazy value

            // enemy is within a good throwing distance ?
            if (distance > (grenadeToThrow == WEAPON_SMGRENADE ? 400 : 600) && distance < 1200)
            {
               // care about different types of grenades
               if (grenadeToThrow == WEAPON_HEGRENADE)
               {
                  bool allowThrowing = true;

                  // check for teammates
                  if (GetNearbyFriendsNearPosition (m_lastEnemy->v.origin, 256) > 0)
                     allowThrowing = false;

                  if (allowThrowing)
                  {
                     Vector enemyPredict = ((m_lastEnemy->v.velocity * 0.5).SkipZ () + m_lastEnemy->v.origin);
                     int searchTab[4], count = 4;

                     float searchRadius = m_lastEnemy->v.velocity.GetLength2D ();

                     // check the search radius
                     if (searchRadius < 128.0f)
                        searchRadius = 128.0f;

                     // search waypoints
                     g_waypoint->FindInRadius (enemyPredict, searchRadius, searchTab, &count);

                     while (count > 0)
                     {
                        allowThrowing = true;

                        // check the throwing
                        m_throw = g_waypoint->GetPath (searchTab[count--])->origin;
                        Vector src = CheckThrow (EyePosition (), m_throw);

                        if (src.GetLengthSquared () < 100.0f)
                           src = CheckToss (EyePosition (), m_throw);

                        if (src == nullvec)
                           allowThrowing = false;
                        else
                           break;
                     }
                  }

                  // start explosive grenade throwing?
                  if (allowThrowing)
                     m_states |= STATE_THROWEXPLODE;
                  else
                     m_states &= ~STATE_THROWEXPLODE;
               }
               else if (grenadeToThrow == WEAPON_SMGRENADE)
               {
                  // start smoke grenade throwing?
                  if ((m_states & STATE_SEEINGENEMY) && GetShootingConeDeviation (m_enemy, &pev->origin) >= 0.90)
                     m_states &= ~STATE_THROWSMOKE;
                  else
                     m_states |= STATE_THROWSMOKE;
               }
            }
         }
         else if (IsAlive (m_lastEnemy) && grenadeToThrow == WEAPON_FBGRENADE && (m_lastEnemy->v.origin - pev->origin).GetLength () < 800 && !(m_aimFlags & AIM_ENEMY) && engine->RandomInt (0, 100) < 60)
         {
            bool allowThrowing = true;
            Array <int> inRadius;

            g_waypoint->FindInRadius (inRadius, 256, m_lastEnemy->v.origin + (m_lastEnemy->v.velocity * 0.5).SkipZ ());

            ITERATE_ARRAY (inRadius, i)
            {
               if (GetNearbyFriendsNearPosition (g_waypoint->GetPath (i)->origin, 256) != 0)
                  continue;

               m_throw = g_waypoint->GetPath (i)->origin;
               Vector src = CheckThrow (EyePosition (), m_throw);

               if (src.GetLengthSquared () < 100)
                  src = CheckToss (EyePosition (), m_throw);

               if (src == nullvec)
                  continue;

               allowThrowing = true;
               break;
            }

            // start concussion grenade throwing?
            if (allowThrowing)
               m_states |= STATE_THROWFLASH;
            else
               m_states &= ~STATE_THROWFLASH;
         }
         const float randTime = engine->GetTime () + engine->RandomFloat (1.5, 3.0f);

         if (m_states & STATE_THROWEXPLODE)
            PushTask (TASK_THROWHEGRENADE, TASKPRI_THROWGRENADE, -1, randTime, false);
         else if (m_states & STATE_THROWFLASH)
            PushTask (TASK_THROWFBGRENADE, TASKPRI_THROWGRENADE, -1, randTime, false);
         else if (m_states & STATE_THROWSMOKE)
            PushTask (TASK_THROWSMGRENADE, TASKPRI_THROWGRENADE, -1, randTime, false);

         // delay next grenade throw
         if (m_states & (STATE_THROWEXPLODE | STATE_THROWFLASH | STATE_THROWSMOKE))
            m_grenadeCheckTime = engine->GetTime () + Const_GrenadeTimer;
      }
   }
   else
      m_states &= ~(STATE_THROWEXPLODE | STATE_THROWFLASH | STATE_THROWSMOKE);

   // check if there are items needing to be used/collected
   if (m_itemCheckTime < engine->GetTime () || !FNullEnt (m_pickupItem))
   {
      m_itemCheckTime = engine->GetTime () + yb_timerpickup.GetFloat ();
      FindItem ();
   }

   float tempFear = m_fearLevel;
   float tempAgression = m_agressionLevel;

   // decrease fear if teammates near
   int friendlyNum = 0;

   if (m_lastEnemyOrigin != nullvec)
      friendlyNum = GetNearbyFriendsNearPosition (pev->origin, 300) - GetNearbyEnemiesNearPosition (m_lastEnemyOrigin, 500);

   if (friendlyNum > 0)
      tempFear = tempFear * 0.5f;

   // increase/decrease fear/aggression if bot uses a sniping weapon to be more careful
   if (UsesSniper ())
   {
      tempFear = tempFear * 1.5f;
      tempAgression = tempAgression * 0.5f;
   }

   // initialize & calculate the desire for all actions based on distances, emotions and other stuff
   GetCurrentTask ();

   // bot found some item to use?
   if (m_pickupType != PICKTYPE_NONE && !FNullEnt (m_pickupItem) && !(m_states & STATE_SEEINGENEMY))
   {
      m_states |= STATE_PICKUPITEM;

      if (m_pickupType == PICKTYPE_BUTTON)
         g_taskFilters[TASK_PICKUPITEM].desire = 50.0f; // always pickup button
      else
      {
         float distance = (500.0f - (GetEntityOrigin (m_pickupItem) - pev->origin).GetLength ()) * 0.2f;

         if (distance > 60.0f)
            distance = 60.0f;

         g_taskFilters[TASK_PICKUPITEM].desire = distance;
      }
   }
   else
   {
      m_states &= ~STATE_PICKUPITEM;
      g_taskFilters[TASK_PICKUPITEM].desire = 0.0f;
   }

   float desireLevel = 0.0f;

   // calculate desire to attack
   if ((m_states & STATE_SEEINGENEMY) && ReactOnEnemy ())
      g_taskFilters[TASK_FIGHTENEMY].desire = TASKPRI_FIGHTENEMY;
   else
      g_taskFilters[TASK_FIGHTENEMY].desire = 0;

   // calculate desires to seek cover or hunt
   if (IsValidPlayer (m_lastEnemy) && m_lastEnemyOrigin != nullvec && !((g_mapType & MAP_DE) && g_bombPlanted) && !(pev->weapons & (1 << WEAPON_C4)) && (m_loosedBombWptIndex == -1 && GetTeam (GetEntity ()) == TEAM_TERRORIST))
   {
      float distance = (m_lastEnemyOrigin - pev->origin).GetLength ();

      // retreat level depends on bot health
      float retreatLevel = (100.0f - (pev->health > 100.0f ? 100.0f : pev->health)) * tempFear;
      float timeSeen = m_seeEnemyTime - engine->GetTime ();
      float timeHeard = m_heardSoundTime - engine->GetTime ();
      float ratio = 0.0f;

      if (timeSeen > timeHeard)
      {
         timeSeen += 10.0f;
         ratio = timeSeen * 0.1f;
      }
      else
      {
         timeHeard += 10.0f;
         ratio = timeHeard * 0.1f;
      }

      if (g_bombPlanted || m_isStuck)
         ratio /= 3; // reduce the seek cover desire if bomb is planted
      else if (m_isVIP || m_isReloading)
         ratio *= 2; // triple the seek cover desire if bot is VIP or reloading

      if (distance > 500.0f)
         g_taskFilters[TASK_SEEKCOVER].desire = retreatLevel * ratio;

      // if half of the round is over, allow hunting
      // FIXME: it probably should be also team/map dependant
      if (FNullEnt (m_enemy) && (g_timeRoundMid < engine->GetTime ()) && !m_isUsingGrenade && m_currentWaypointIndex != g_waypoint->FindNearest (m_lastEnemyOrigin) && m_personality != PERSONALITY_CAREFUL)
      {
         desireLevel = 4096.0f - ((1.0f - tempAgression) * distance);
         desireLevel = (100 * desireLevel) / 4096.0f;
         desireLevel -= retreatLevel;

         if (desireLevel > 89)
            desireLevel = 89;

         g_taskFilters[TASK_HUNTENEMY].desire = desireLevel;
      }
      else
         g_taskFilters[TASK_HUNTENEMY].desire = 0;
   }
   else
   {
      g_taskFilters[TASK_SEEKCOVER].desire = 0;
      g_taskFilters[TASK_HUNTENEMY].desire = 0;
   }

   // blinded behaviour
   if (m_blindTime > engine->GetTime ())
      g_taskFilters[TASK_BLINDED].desire = TASKPRI_BLINDED;
   else
      g_taskFilters[TASK_BLINDED].desire = 0.0f;

   // now we've initialized all the desires go through the hard work
   // of filtering all actions against each other to pick the most
   // rewarding one to the bot.

   // FIXME: instead of going through all of the actions it might be
   // better to use some kind of decision tree to sort out impossible
   // actions.

   // most of the values were found out by trial-and-error and a helper
   // utility i wrote so there could still be some weird behaviors, it's
   // hard to check them all out.

   m_oldCombatDesire = HysteresisDesire (g_taskFilters[TASK_FIGHTENEMY].desire, 40.0, 90.0, m_oldCombatDesire);
   g_taskFilters[TASK_FIGHTENEMY].desire = m_oldCombatDesire;

   Task *taskOffensive = &g_taskFilters[TASK_FIGHTENEMY];
   Task *taskPickup = &g_taskFilters[TASK_PICKUPITEM];

   // calc survive (cover/hide)
   Task *taskSurvive = ThresholdDesire (&g_taskFilters[TASK_SEEKCOVER], 40.0, 0.0f);
   taskSurvive = SubsumeDesire (&g_taskFilters[TASK_HIDE], taskSurvive);

   Task *def = ThresholdDesire (&g_taskFilters[TASK_HUNTENEMY], 60.0, 0.0f); // don't allow hunting if desire's 60<
   taskOffensive = SubsumeDesire (taskOffensive, taskPickup); // if offensive task, don't allow picking up stuff

   Task *taskSub = MaxDesire (taskOffensive, def); // default normal & careful tasks against offensive actions
   Task *final = SubsumeDesire (&g_taskFilters[TASK_BLINDED], MaxDesire (taskSurvive, taskSub)); // reason about fleeing instead

   if (m_tasks != null)
      final = MaxDesire (final, m_tasks);

   PushTask (final); // push the final behaviour in our task stack to carry out
}

void Bot::ResetTasks (void)
{
   // this function resets bot tasks stack, by removing all entries from the stack.

   if (m_tasks == null)
      return; // reliability check

   Task *next = m_tasks->next;
   Task *prev = m_tasks;

   while (m_tasks != null)
   {
      prev = m_tasks->prev;

      delete m_tasks;
      m_tasks = prev;
   }
   m_tasks = next;

   while (m_tasks != null)
   {
      next = m_tasks->next;

      delete m_tasks;
      m_tasks = next;
   }
   m_tasks = null;
}

void Bot::CheckTasksPriorities (void)
{
   // this function checks the tasks priorities.

   if (m_tasks == null)
   {
      GetCurrentTask ();
      return;
   }

   Task *oldTask = m_tasks;
   Task *prev = null;
   Task *next = null;

   while (m_tasks->prev != null)
   {
      prev = m_tasks->prev;
      m_tasks = prev;
   }

   Task *firstTask = m_tasks;
   Task *maxDesiredTask = m_tasks;

   float maxDesire = m_tasks->desire;

   // search for the most desired task
   while (m_tasks->next != null)
   {
      next = m_tasks->next;
      m_tasks = next;

      if (m_tasks->desire >= maxDesire)
      {
         maxDesiredTask = m_tasks;
         maxDesire = m_tasks->desire;
      }
   }
   m_tasks = maxDesiredTask; // now we found the most desired pushed task...

   // something was changed with priorities, check if some task doesn't need to be deleted...
   if (oldTask != maxDesiredTask)
   {
      m_tasks = firstTask;

      while (m_tasks != null)
      {
         next = m_tasks->next;

         // some task has to be deleted if cannot be continued...
         if (m_tasks != maxDesiredTask && !m_tasks->canContinue)
         {
            if (m_tasks->prev != null)
               m_tasks->prev->next = m_tasks->next;

            if (m_tasks->next != null)
               m_tasks->next->prev = m_tasks->prev;

            delete m_tasks;
         }
         m_tasks = next;
      }
   }
   m_tasks = maxDesiredTask;

   if (yb_debuggoal.GetInt () != -1)
      m_chosenGoalIndex = yb_debuggoal.GetInt ();
   else
      m_chosenGoalIndex = m_tasks->data;
}

void Bot::PushTask (BotTask taskID, float desire, int data, float time, bool canContinue)
{
   // build task
   Task task = {null, null, taskID, desire, data, time, canContinue};
   PushTask (&task); // use standard function to start task
}

void Bot::PushTask (Task *task)
{
   // this function adds task pointer on the bot task stack.

   bool newTaskDifferent = false;
   bool foundTaskExisting = false;
   bool checkPriorities = false;

   Task *oldTask = GetCurrentTask (); // remember our current task

   // at the beginning need to clean up all null tasks...
   if (m_tasks == null)
   {
      m_lastCollTime = engine->GetTime () + 0.5f;

      Task *newTask = new Task;

      newTask->taskID = TASK_NORMAL;
      newTask->desire = TASKPRI_NORMAL;
      newTask->canContinue = true;
      newTask->time = 0.0f;
      newTask->data = -1;
      newTask->next = newTask->prev = null;

      m_tasks = newTask;
      DeleteSearchNodes ();

      if (task == null)
         return;
      else if (task->taskID == TASK_NORMAL)
      {
         m_tasks->desire = TASKPRI_NORMAL;
         m_tasks->data = task->data;
         m_tasks->time = task->time;
         
         return;
      }
   }
   else if (task == null)
      return;

   // it shouldn't happen this condition now as false...
   if (m_tasks != null)
   {
      if (m_tasks->taskID == task->taskID)
      {
         if (m_tasks->data != task->data)
         {
            m_lastCollTime = engine->GetTime () + 0.5f;

            DeleteSearchNodes ();
            m_tasks->data = task->data;
         }

         if (m_tasks->desire != task->desire)
         {
            m_tasks->desire = task->desire;
            checkPriorities = true;
         }
         else if (m_tasks->data == task->data)
            return;
      }
      else
      {
         // find the first task on the stack and don't allow push the new one like the same already existing one
         while (m_tasks->prev != null)
         {
            m_tasks = m_tasks->prev;

            if (m_tasks->taskID == task->taskID)
            {
               foundTaskExisting = true;

               if (m_tasks->desire != task->desire)
                  checkPriorities = true;

               m_tasks->desire = task->desire;
               m_tasks->data = task->data;
               m_tasks->time = task->time;
               m_tasks->canContinue = task->canContinue;
               m_tasks = oldTask;

               break; // now we may need to check the current max desire or next tasks...
            }
         }

         // now go back to the previous stack position and try to find the same task as one of "the next" ones (already pushed before and not finished yet)
         if (!foundTaskExisting && !checkPriorities)
         {
            m_tasks = oldTask;

            while (m_tasks->next != null)
            {
               m_tasks = m_tasks->next;

               if (m_tasks->taskID == task->taskID)
               {
                  foundTaskExisting = true;

                  if (m_tasks->desire != task->desire)
                     checkPriorities = true;

                  m_tasks->desire = task->desire;
                  m_tasks->data = task->data;
                  m_tasks->time = task->time;
                  m_tasks->canContinue = task->canContinue;
                  m_tasks = oldTask;

                  break; // now we may need to check the current max desire...
               }
            }
         }

         if (!foundTaskExisting)
            newTaskDifferent = true; // we have some new task pushed on the stack...
      }
   }
   m_tasks = oldTask;

   if (newTaskDifferent)
   {
      Task *newTask = new Task;

      newTask->taskID = task->taskID;
      newTask->desire = task->desire;
      newTask->canContinue = task->canContinue;
      newTask->time = task->time;
      newTask->data = task->data;
      newTask->next = null;

      while (m_tasks->next != null)
         m_tasks = m_tasks->next;

      newTask->prev = m_tasks;
      m_tasks->next = newTask;

      checkPriorities = true;
   }
   m_tasks = oldTask;

   // needs check the priorities and setup the task with the max desire...
   if (!checkPriorities)
      return;
 
   CheckTasksPriorities ();

   // the max desired task has been changed...
   if (m_tasks != oldTask)
   {
      DeleteSearchNodes ();
      m_lastCollTime = engine->GetTime () + 0.5f;

      // leader bot?
      if (newTaskDifferent && m_isLeader && m_tasks->taskID == TASK_SEEKCOVER)
         CommandTeam (); // reorganize team if fleeing

      if (newTaskDifferent && m_tasks->taskID == TASK_CAMP)
         SelectBestWeapon ();

      // this is best place to handle some voice commands report team some info
      if (newTaskDifferent && engine->RandomInt (0, 100) < 95)
      {
         switch (m_tasks->taskID)
         {
         case TASK_BLINDED:
            ChatterMessage (Chatter_GotBlinded);
            break;

         case TASK_PLANTBOMB:
            ChatterMessage (Chatter_PlantingC4);
            break;
         }
      }

      if (newTaskDifferent && engine->RandomInt (0, 100) < 80 && m_tasks->taskID == TASK_CAMP)
      {
         if ((g_mapType & MAP_DE) && g_bombPlanted)
            ChatterMessage (Chatter_GuardDroppedC4);
         else
            ChatterMessage (Chatter_GoingToCamp);
      }

      if (newTaskDifferent && engine->RandomInt (0, 100) < 80 && m_tasks->taskID == TASK_CAMP && GetTeam (GetEntity ()) == TEAM_TERRORIST && m_inVIPZone)
         ChatterMessage (Chatter_GoingToGuardVIPSafety);
   }
}

Task *Bot::GetCurrentTask (void)
{
   if (m_tasks != null)
      return m_tasks;

   Task *newTask = new Task;

   newTask->taskID = TASK_NORMAL;
   newTask->desire = TASKPRI_NORMAL;
   newTask->data = -1;
   newTask->time = 0.0f;
   newTask->canContinue = true;
   newTask->next = newTask->prev = null;

   m_tasks = newTask;
   m_lastCollTime = engine->GetTime () + 0.5f;

   DeleteSearchNodes ();
   return m_tasks;
}

void Bot::RemoveCertainTask (BotTask taskID)
{
   // this function removes one task from the bot task stack.

   if (m_tasks == null || (m_tasks != null && m_tasks->taskID == TASK_NORMAL))
      return; // since normal task can be only once on the stack, don't remove it...

   bool checkPriorities = false;

   Task *task = m_tasks;
   Task *oldTask = m_tasks;
   Task *oldPrevTask = task->prev;
   Task *oldNextTask = task->next;

   while (task->prev != null)
      task = task->prev;

   while (task != null)
   {
      Task *next = task->next;
      Task *prev = task->prev;

      if (task->taskID == taskID)
      {
         if (prev != null)
            prev->next = next;

         if (next != null)
            next->prev = prev;

         if (task == oldTask)
            oldTask = null;
         else if (task == oldPrevTask)
            oldPrevTask = null;
         else if (task == oldNextTask)
            oldNextTask = null;

         delete task;

         checkPriorities = true;
         break;
      }
      task = next;
   }

   if (oldTask != null)
      m_tasks = oldTask;
   else if (oldPrevTask != null)
      m_tasks = oldPrevTask;
   else if (oldNextTask != null)
      m_tasks = oldNextTask;
   else
      GetCurrentTask ();

   if (checkPriorities)
      CheckTasksPriorities ();
}

void Bot::TaskComplete (void)
{
   // this function called whenever a task is completed.

   if (m_tasks == null)
   {
      DeleteSearchNodes (); // delete all path finding nodes
      return;
   }

   if (m_tasks->taskID == TASK_NORMAL)
   {
      DeleteSearchNodes (); // delete all path finding nodes
      
      m_tasks->data = -1;
      m_chosenGoalIndex = -1;

      return;
   }

   Task *next = m_tasks->next;
   Task *prev = m_tasks->prev;

   if (next != null)
      next->prev = prev;
   
   if (prev != null)
      prev->next = next;

   delete m_tasks;
   m_tasks = null;

   if (prev != null && next != null)
   {
      if (prev->desire >= next->desire)
         m_tasks = prev;
      else
         m_tasks = next;
   }
   else if (prev != null)
      m_tasks = prev;
   else if (next != null)
      m_tasks = next;

   if (m_tasks == null)
      GetCurrentTask ();

   CheckTasksPriorities ();
   DeleteSearchNodes ();
}

bool Bot::EnemyIsThreat (void)
{
   if (FNullEnt (m_enemy) || GetCurrentTask ()->taskID == TASK_SEEKCOVER)
      return false;

   float distance = (m_enemy->v.origin - pev->origin).GetLength ();

   // if bot is camping, he should be firing anyway and not leaving his position
   if (GetCurrentTask ()->taskID == TASK_CAMP)
      return false;

   // if enemy is near or facing us directly
   if (distance < 256 || IsInViewCone (m_enemy->v.origin))
      return true;

   return false;
}

bool Bot::ReactOnEnemy (void)
{
   // the purpose of this function is check if task has to be interrupted because an enemy is near (run attack actions then)

   if (!EnemyIsThreat ())
      return false;

   if (m_enemyReachableTimer < engine->GetTime ())
   {
      int i = g_waypoint->FindNearest (pev->origin);
      int enemyIndex = g_waypoint->FindNearest (m_enemy->v.origin);

      float lineDist = (m_enemy->v.origin - pev->origin).GetLength ();
      float pathDist = g_waypoint->GetPathDistanceFloat (i, enemyIndex);

      if (pathDist - lineDist > 112.0f)
         m_isEnemyReachable = false;
      else
         m_isEnemyReachable = true;

      m_enemyReachableTimer = engine->GetTime () + 1.0f;
   }

   if (m_isEnemyReachable)
   {
      m_navTimeset = engine->GetTime (); // override existing movement by attack movement
      return true;
   }
   return false;
}

bool Bot::IsLastEnemyViewable (void)
{
   // this function checks if line of sight established to last enemy

   if (FNullEnt (m_lastEnemy) || m_lastEnemyOrigin == nullvec)
   {
      m_lastEnemy = null;
      m_lastEnemyOrigin = nullvec;

      return false;
   }

   // trace a line from bot's eyes to destination...
   TraceResult tr;
   TraceLine (EyePosition (), m_lastEnemyOrigin, true, GetEntity (), &tr);

   // check if line of sight to object is not blocked (i.e. visible)
   return tr.flFraction >= 1.0f;
}

bool Bot::LastEnemyShootable (void)
{
   // don't allow shooting through walls when pausing or camping
   if (!(m_aimFlags & AIM_LASTENEMY) || FNullEnt (m_lastEnemy) || GetCurrentTask ()->taskID == TASK_PAUSE || GetCurrentTask ()->taskID == TASK_CAMP)
      return false;

   return GetShootingConeDeviation (GetEntity (), &m_lastEnemyOrigin) >= 0.90;
}

void Bot::CheckRadioCommands (void)
{
   // this function handling radio and reactings to it

   float distance = (m_radioEntity->v.origin - pev->origin).GetLength ();

   // don't allow bot listen you if bot is busy
   if (GetCurrentTask ()->taskID == TASK_DEFUSEBOMB || GetCurrentTask ()->taskID == TASK_PLANTBOMB || HasHostage () && m_radioOrder != Radio_ReportTeam)
   {
      m_radioOrder = 0;
      return;
   }

   switch (m_radioOrder)
   {
   case Radio_CoverMe:
   case Radio_FollowMe:
   case Chatter_GoingToPlantBomb:
      // check if line of sight to object is not blocked (i.e. visible)
      if (EntityIsVisible (m_radioEntity->v.origin))
      {
         if (FNullEnt (m_targetEntity) && FNullEnt (m_enemy) && engine->RandomInt (0, 100) < (m_personality == PERSONALITY_CAREFUL ? 80 : 50))
         {
            int numFollowers = 0;

            // Check if no more followers are allowed
            for (int i = 0; i < engine->GetMaxClients (); i++)
            {
               if (g_botManager->GetBot (i))
               {
                  if (IsAlive (g_botManager->GetBot (i)->GetEntity ()))
                  {
                     if (g_botManager->GetBot (i)->m_targetEntity == m_radioEntity)
                        numFollowers++;
                  }
               }
            }

            int allowedFollowers = yb_followuser.GetInt ();

            if (m_radioEntity->v.weapons & (1 << WEAPON_C4))
               allowedFollowers = 1;

            if (numFollowers < allowedFollowers)
            {
               RadioMessage (Radio_Affirmative);
               m_targetEntity = m_radioEntity;

               // don't pause/camp/follow anymore
               BotTask taskID = GetCurrentTask ()->taskID;

               if (taskID == TASK_PAUSE || taskID == TASK_CAMP)
                  m_tasks->time = engine->GetTime ();

               PushTask (TASK_FOLLOWUSER, TASKPRI_FOLLOWUSER, -1, 0.0, true);
            }
            else if (numFollowers > allowedFollowers)
            {
               for (int i = 0; (i < engine->GetMaxClients () && numFollowers > allowedFollowers) ; i++)
               {
                  if (g_botManager->GetBot (i))
                  {
                     if (IsAlive (g_botManager->GetBot (i)->GetEntity ()))
                     {
                        if (g_botManager->GetBot (i)->m_targetEntity == m_radioEntity)
                        {
                           g_botManager->GetBot (i)->m_targetEntity = null;
                           numFollowers--;
                        }
                     }
                  }
               }
            }
            else if (m_radioOrder != Chatter_GoingToPlantBomb)
               RadioMessage (Radio_Negative);
         }
         else if (m_radioOrder != Chatter_GoingToPlantBomb)
            RadioMessage (Radio_Negative);
      }
      break;

   case Radio_HoldPosition:
      if (!FNullEnt (m_targetEntity))
      {
         if (m_targetEntity == m_radioEntity)
         {
            m_targetEntity = null;
            RadioMessage (Radio_Affirmative);

            m_campButtons = 0;

            PushTask (TASK_PAUSE, TASKPRI_PAUSE, -1, engine->GetTime () + engine->RandomFloat (30.0, 60.0f), false);
         }
      }
      break;

   case Radio_TakingFire:
      if (FNullEnt (m_targetEntity))
      {
         if (FNullEnt (m_enemy))
         {
            // Decrease Fear Levels to lower probability of Bot seeking Cover again
            m_fearLevel -= 0.2f;

            if (m_fearLevel < 0.0f)
               m_fearLevel = 0.0f;

            if (engine->RandomInt (0, 100) < 45 && yb_commtype.GetInt () == 2)
               ChatterMessage (Chatter_OnMyWay);
            else if (m_radioOrder == Radio_NeedBackup && yb_commtype.GetInt () != 2)
               RadioMessage (Radio_Affirmative);

            // don't pause/camp anymore
            BotTask taskID = GetCurrentTask ()->taskID;

            if (taskID == TASK_PAUSE || taskID == TASK_CAMP)
               m_tasks->time = engine->GetTime ();

            m_position = m_radioEntity->v.origin;
            DeleteSearchNodes ();

            PushTask (TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, -1, 0.0, true);
         }
         else if (engine->RandomInt (0, 100) < 80 && m_radioOrder == Radio_TakingFire)
            RadioMessage (Radio_Negative);
      }
      break;

   case Radio_YouTakePoint:
      if (EntityIsVisible (m_radioEntity->v.origin) && m_isLeader)
         RadioMessage (Radio_Affirmative);
      break;

   case Radio_NeedBackup:
   case Chatter_ScaredEmotion:
      if ((FNullEnt (m_enemy) && EntityIsVisible (m_radioEntity->v.origin)) || distance < 2048 || !m_moveToC4)
      {
         m_fearLevel -= 0.1f;

         if (m_fearLevel < 0.0f)
            m_fearLevel = 0.0f;

         if (engine->RandomInt (0, 100) < 45 && yb_commtype.GetInt () == 2)
            ChatterMessage (Chatter_OnMyWay);
         else if (m_radioOrder == Radio_NeedBackup && yb_commtype.GetInt () != 2)
            RadioMessage (Radio_Affirmative);

         // don't pause/camp anymore
         BotTask taskID = GetCurrentTask ()->taskID;

         if (taskID == TASK_PAUSE || taskID == TASK_CAMP)
            m_tasks->time = engine->GetTime ();

         m_moveToC4 = true;
         m_position = m_radioEntity->v.origin;

         DeleteSearchNodes ();

         // start move to position task
         PushTask (TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, -1, 0.0, true);
      }
      else if (engine->RandomInt (0, 100) < 80 && m_radioOrder == Radio_NeedBackup)
         RadioMessage (Radio_Negative);
      break;

   case Radio_GoGoGo:
      if (m_radioEntity == m_targetEntity)
      {
         RadioMessage (Radio_Affirmative);

         m_targetEntity = null;
         m_fearLevel -= 0.3f;

         if (m_fearLevel < 0.0f)
            m_fearLevel = 0.0f;
      }
      else if ((FNullEnt (m_enemy) && EntityIsVisible (m_radioEntity->v.origin)) || distance < 2048)
      {
         BotTask taskID = GetCurrentTask ()->taskID;

         if (taskID == TASK_PAUSE || taskID == TASK_CAMP)
         {
            m_fearLevel -= 0.3f;

            if (m_fearLevel < 0.0f)
               m_fearLevel = 0.0f;

            RadioMessage (Radio_Affirmative);
            // don't pause/camp anymore
            m_tasks->time = engine->GetTime ();

            m_targetEntity = null;
            MakeVectors (m_radioEntity->v.v_angle);

            m_position = m_radioEntity->v.origin + g_pGlobals->v_forward * engine->RandomFloat (1024.0f, 2048.0f);

            DeleteSearchNodes ();
            PushTask (TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, -1, 0.0f, true);
         }
      }
      else if (!FNullEnt (m_doubleJumpEntity))
      {
         RadioMessage (Radio_Affirmative);
         ResetDoubleJumpState ();
      }
      else
         RadioMessage (Radio_Negative);
      break;

   case Radio_ShesGonnaBlow:
      if (FNullEnt (m_enemy) && distance < 2048 && g_bombPlanted && GetTeam (GetEntity ()) == TEAM_TERRORIST)
      {
         RadioMessage (Radio_Affirmative);

         if (GetCurrentTask ()->taskID == TASK_CAMP)
            RemoveCertainTask (TASK_CAMP);

         m_targetEntity = null;
         PushTask (TASK_ESCAPEFROMBOMB, TASKPRI_ESCAPEFROMBOMB, -1, 0.0, true);
      }
      else
        RadioMessage (Radio_Negative);

      break;

   case Radio_RegroupTeam:
      // if no more enemies found AND bomb planted, switch to knife to get to bombplace faster
      if ((GetTeam (GetEntity ()) == TEAM_COUNTER) && m_currentWeapon != WEAPON_KNIFE && GetNearbyEnemiesNearPosition (pev->origin, 9999) == 0 && g_bombPlanted && GetCurrentTask ()->taskID != TASK_DEFUSEBOMB)
      {
         SelectWeaponByName ("weapon_knife");

         DeleteSearchNodes ();
         MoveToVector (g_waypoint->GetBombPosition ());

         RadioMessage (Radio_Affirmative);
      }
      break;

   case Radio_StormTheFront:
      if ((FNullEnt (m_enemy) && EntityIsVisible (m_radioEntity->v.origin)) || distance < 1024)
      {
         RadioMessage (Radio_Affirmative);

         // don't pause/camp anymore
         BotTask taskID = GetCurrentTask ()->taskID;

         if (taskID == TASK_PAUSE || taskID == TASK_CAMP)
            m_tasks->time = engine->GetTime ();

         m_targetEntity = null;

         MakeVectors (m_radioEntity->v.v_angle);
         m_position = m_radioEntity->v.origin + g_pGlobals->v_forward * engine->RandomFloat (1024.0f, 2048.0f);

         DeleteSearchNodes ();
         PushTask (TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, -1, 0.0f, true);

         m_fearLevel -= 0.3f;

         if (m_fearLevel < 0.0f)
            m_fearLevel = 0.0f;

         m_agressionLevel += 0.3f;

         if (m_agressionLevel > 1.0f)
            m_agressionLevel = 1.0f;
      }
      break;

   case Radio_Fallback:
      if ((FNullEnt (m_enemy) && EntityIsVisible (m_radioEntity->v.origin)) || distance < 1024)
      {
         m_fearLevel += 0.5f;

         if (m_fearLevel > 1.0f)
            m_fearLevel = 1.0f;

         m_agressionLevel -= 0.5f;

         if (m_agressionLevel < 0.0f)
            m_agressionLevel = 0.0f;

         if (GetCurrentTask ()->taskID == TASK_CAMP)
            m_tasks->time += engine->RandomFloat (10.0, 15.0f);
         else
         {
            // don't pause/camp anymore
            BotTask taskID = GetCurrentTask ()->taskID;

            if (taskID == TASK_PAUSE)
               m_tasks->time = engine->GetTime ();

            m_targetEntity = null;
            m_seeEnemyTime = engine->GetTime ();

            // if bot has no enemy
            if (m_lastEnemyOrigin == nullvec)
            {
               int team = GetTeam (GetEntity ());
               float nearestDistance = FLT_MAX;

               // take nearest enemy to ordering player
               for (int i = 0; i < engine->GetMaxClients (); i++)
               {
                  if (!(g_clients[i].flags & CFLAG_USED) || !(g_clients[i].flags & CFLAG_ALIVE) || g_clients[i].team == team)
                     continue;

                  edict_t *enemy = g_clients[i].ent;
                  float nowDistance = (m_radioEntity->v.origin - enemy->v.origin).GetLengthSquared ();

                  if (nowDistance < nearestDistance)
                  {
                     nearestDistance = nowDistance;

                     m_lastEnemy = enemy;
                     m_lastEnemyOrigin = enemy->v.origin;
                  }
               }
            }
            DeleteSearchNodes ();
         }
      }
      break;

   case Radio_ReportTeam:
      if (engine->RandomInt  (0, 100) < 85)
         RadioMessage ((GetNearbyEnemiesNearPosition (pev->origin, 400) == 0 && yb_commtype.GetInt () != 2) ? Radio_SectorClear : Radio_ReportingIn);
      break;

   case Radio_SectorClear:
      // is bomb planted and it's a ct
      if (g_bombPlanted)
      {
         int bombPoint = -1;

         // check if it's a ct command
         if (GetTeam (m_radioEntity) == TEAM_COUNTER && GetTeam (GetEntity ()) == TEAM_COUNTER && IsValidBot (m_radioEntity))
         {
            if (g_timeNextBombUpdate < engine->GetTime ())
            {
               float minDistance = FLT_MAX;

               // find nearest bomb waypoint to player
               ITERATE_ARRAY (g_waypoint->m_goalPoints, i)
               {
                  distance = (g_waypoint->GetPath (g_waypoint->m_goalPoints[i])->origin - m_radioEntity->v.origin).GetLengthSquared ();

                  if (distance < minDistance)
                  {
                     minDistance = distance;
                     bombPoint = g_waypoint->m_goalPoints[i];
                  }
               }

               // mark this waypoint as restricted point
               if (bombPoint != -1 && !g_waypoint->IsGoalVisited (bombPoint))
                  g_waypoint->SetGoalVisited (bombPoint);

               g_timeNextBombUpdate = engine->GetTime () + 0.5f;
            }
            // Does this Bot want to defuse?
            if (GetCurrentTask ()->taskID == TASK_NORMAL)
            {
               // Is he approaching this goal?
               if (m_tasks->data == bombPoint)
               {
                  m_tasks->data = -1;
                  RadioMessage (Radio_Affirmative);
               }
            }
         }
      }
      break;

   case Radio_GetInPosition:
      if ((FNullEnt (m_enemy) && EntityIsVisible (m_radioEntity->v.origin)) || distance < 1024)
      {
         RadioMessage (Radio_Affirmative);

         if (GetCurrentTask ()->taskID == TASK_CAMP)
            m_tasks->time = engine->GetTime () + engine->RandomFloat (30.0, 60.0f);
         else
         {
            // don't pause anymore
            BotTask taskID = GetCurrentTask ()->taskID;

            if (taskID == TASK_PAUSE)
               m_tasks->time = engine->GetTime ();

            m_targetEntity = null;
            m_seeEnemyTime = engine->GetTime ();

            // If Bot has no enemy
            if (m_lastEnemyOrigin == nullvec)
            {
               int team = GetTeam (GetEntity ());
               float nearestDistance = FLT_MAX;

               // Take nearest enemy to ordering Player
               for (int i = 0; i < engine->GetMaxClients (); i++)
               {
                  if (!(g_clients[i].flags & CFLAG_USED) || !(g_clients[i].flags & CFLAG_ALIVE) || g_clients[i].team == team)
                     continue;

                  edict_t *enemy = g_clients[i].ent;
                  float nowDistance = (m_radioEntity->v.origin - enemy->v.origin).GetLengthSquared ();

                  if (nowDistance < nearestDistance)
                  {
                     nearestDistance = nowDistance;
                     m_lastEnemy = enemy;
                     m_lastEnemyOrigin = enemy->v.origin;
                  }
               }
            }
            DeleteSearchNodes ();

            int index = FindDefendWaypoint (m_radioEntity->v.origin);

            // push camp task on to stack
            PushTask (TASK_CAMP, TASKPRI_CAMP, -1, engine->GetTime () + engine->RandomFloat (30.0, 60.0f), true);
            // push move command
            PushTask (TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, index, engine->GetTime () + engine->RandomFloat (30.0, 60.0f), true);

            if (g_waypoint->GetPath (index)->vis.crouch <= g_waypoint->GetPath (index)->vis.stand)
               m_campButtons |= IN_DUCK;
            else
               m_campButtons &= ~IN_DUCK;
         }
      }
      break;
   }
   m_radioOrder = 0; // radio command has been handled, reset
}

void Bot::SelectLeaderEachTeam (int team)
{
   Bot *botLeader = null;

   if (g_mapType & MAP_AS)
   {
      if (m_isVIP && !g_leaderChoosen[TEAM_COUNTER])
      {
         // vip bot is the leader
         m_isLeader = true;

         if (engine->RandomInt (1, 100) < 50)
         {
            RadioMessage (Radio_FollowMe);
            m_campButtons = 0;
         }
         g_leaderChoosen[TEAM_COUNTER] = true;
      }
      else if ((team == TEAM_TERRORIST) && !g_leaderChoosen[TEAM_TERRORIST])
      {
         botLeader = g_botManager->GetHighestFragsBot (team);

         if (botLeader != null)
         {
            botLeader->m_isLeader = true;

            if (engine->RandomInt (1, 100) < 45)
               botLeader->RadioMessage (Radio_FollowMe);
         }
         g_leaderChoosen[TEAM_TERRORIST] = true;
      }
   }
   else if (g_mapType & MAP_DE)
   {
      if (team == TEAM_TERRORIST && !g_leaderChoosen[TEAM_TERRORIST])
      {
         if (pev->weapons & (1 << WEAPON_C4))
         {
            // bot carrying the bomb is the leader
            m_isLeader = true;

            // terrorist carrying a bomb needs to have some company
            if (engine->RandomInt (1, 100) < 80)
            {
               if (yb_commtype.GetInt () == 2)
                  ChatterMessage (Chatter_GoingToPlantBomb);
               else
                  ChatterMessage (Radio_FollowMe);

               m_campButtons = 0;
            }
            g_leaderChoosen[TEAM_TERRORIST] = true;
         }
      }
      else if (!g_leaderChoosen[TEAM_COUNTER])
      {
         botLeader = g_botManager->GetHighestFragsBot (team);

         if (botLeader != null)
         {
            botLeader->m_isLeader = true;

            if (engine->RandomInt (1, 100) < 30)
               botLeader->RadioMessage (Radio_FollowMe);
         }
         g_leaderChoosen[TEAM_COUNTER] = true;
      }
   }
   else if (g_mapType & (MAP_ES | MAP_KA | MAP_FY))
   {
      if (team == TEAM_TERRORIST)
      {
         botLeader = g_botManager->GetHighestFragsBot (team);

         if (botLeader != null)
         {
            botLeader->m_isLeader = true;

            if (engine->RandomInt (1, 100) < 30)
               botLeader->RadioMessage (Radio_FollowMe);
         }
      }
      else
      {
         botLeader = g_botManager->GetHighestFragsBot (team);

         if (botLeader != null)
         {
            botLeader->m_isLeader = true;

            if (engine->RandomInt (1, 100) < 30)
               botLeader->RadioMessage (Radio_FollowMe);
         }
      }
   }
   else
   {
      if (team == TEAM_TERRORIST)
      {
         botLeader = g_botManager->GetHighestFragsBot (team);

         if (botLeader != null)
         {
            botLeader->m_isLeader = true;

            if (engine->RandomInt (1, 100) < 30)
               botLeader->RadioMessage (Radio_FollowMe);
         }
      }
      else
      {
         botLeader = g_botManager->GetHighestFragsBot (team);

         if (botLeader != null)
         {
            botLeader->m_isLeader = true;

            if (engine->RandomInt (1, 100) < 40)
               botLeader->RadioMessage (Radio_FollowMe);
         }
      }
   }
}

void Bot::ChooseAimDirection (void)
{
   if (!m_canChooseAimDirection)
      return;

   TraceResult tr;
   memset (&tr, 0, sizeof (TraceResult));

   unsigned int flags = m_aimFlags;

   bool canChooseAimDirection = false;
   bool currentPointValid = (m_currentWaypointIndex >= 0 && m_currentWaypointIndex < g_numWaypoints);

   if (!currentPointValid)
   {
      currentPointValid = true;
      GetValidWaypoint ();
   }

   // check if last enemy vector valid
   if (m_lastEnemyOrigin != nullvec)
   {
      TraceLine (EyePosition (), m_lastEnemyOrigin, false, true, GetEntity (), &tr);

      if ((pev->origin - m_lastEnemyOrigin).GetLength () >= 1600 && FNullEnt (m_enemy) && !UsesSniper () || (tr.flFraction <= 0.2 && tr.pHit == g_hostEntity) && m_seeEnemyTime + 7.0f < engine->GetTime ())
      {
         if ((m_aimFlags & (AIM_LASTENEMY | AIM_PREDICTENEMY)) && m_wantsToFire)
            m_wantsToFire = false;

         m_lastEnemyOrigin = nullvec;
         m_aimFlags &= ~(AIM_LASTENEMY | AIM_PREDICTENEMY);

         flags &= ~(AIM_LASTENEMY | AIM_PREDICTENEMY);
      }
   }
   else
   {
      m_aimFlags &= ~(AIM_LASTENEMY | AIM_PREDICTENEMY);
      flags &= ~(AIM_LASTENEMY | AIM_PREDICTENEMY);
   }

   // don't allow bot to look at danger positions under certain circumstances
   if (!(flags & (AIM_GRENADE | AIM_ENEMY | AIM_ENTITY)))
   {
      if (IsOnLadder () || IsInWater () || (m_waypointFlags & WAYPOINT_LADDER) || (m_currentTravelFlags & PATHFLAG_JUMP))
      {
         flags &= ~(AIM_LASTENEMY | AIM_PREDICTENEMY);
         canChooseAimDirection = false;
      }
   }

   if (flags & AIM_OVERRIDE)
      m_lookAt = m_camp;
   else if (flags & AIM_GRENADE)
      m_lookAt = m_throw + Vector (0, 0, 1.0f * m_grenade.z);
   else if (flags & AIM_ENEMY)
      FocusEnemy ();
   else if (flags & AIM_ENTITY)
      m_lookAt = m_entity;
   else if (flags & AIM_LASTENEMY)
   {
      m_lookAt = m_lastEnemyOrigin;

      // did bot just see enemy and is quite aggressive?
      if (m_seeEnemyTime + 3.0f - m_actualReactionTime + m_baseAgressionLevel > engine->GetTime ())
      {
         // feel free to fire if shootable
         if (!UsesSniper () && LastEnemyShootable ())
            m_wantsToFire = true;
      }
      else // forget an enemy far away
      {
         m_aimFlags &= ~AIM_LASTENEMY;

         if ((pev->origin - m_lastEnemyOrigin).GetLength () >= 1600.0f)
            m_lastEnemyOrigin = nullvec;
      }
   }
   else if (flags & AIM_PREDICTENEMY)
   {
      if (((pev->origin - m_lastEnemyOrigin).GetLength () < 1600 || UsesSniper ()) && (tr.flFraction >= 0.2 || tr.pHit != g_worldEdict))
      {
         bool recalcPath = true;

         if (!FNullEnt (m_lastEnemy) && m_trackingEdict == m_lastEnemy && m_timeNextTracking < engine->GetTime ())
            recalcPath = false;

         if (recalcPath)
         {
            m_lookAt = g_waypoint->GetPath (GetAimingWaypoint (m_lastEnemyOrigin))->origin;
            m_camp = m_lookAt;

            m_timeNextTracking = engine->GetTime () + 0.5f;
            m_trackingEdict = m_lastEnemy;

            // feel free to fire if shoot able
            if (LastEnemyShootable ())
               m_wantsToFire = true;
         }
         else
            m_lookAt = m_camp;
      }
      else // forget an enemy far away
      {
         m_aimFlags &= ~AIM_PREDICTENEMY;

         if ((pev->origin - m_lastEnemyOrigin).GetLength () >= 1600.0f)
            m_lastEnemyOrigin = nullvec;
      }
   }
   else if (flags & AIM_CAMP)
      m_lookAt = m_camp;
   else if ((flags & AIM_NAVPOINT) && !(flags & (AIM_ENTITY | AIM_ENEMY)))
   {
      m_lookAt = m_destOrigin;

      if (canChooseAimDirection && m_currentWaypointIndex != -1 && !(g_waypoint->GetPath (m_currentWaypointIndex)->flags & WAYPOINT_LADDER))
      {
         int dangerIndex  = g_exp.GetDangerIndex (m_currentWaypointIndex, m_currentWaypointIndex, GetTeam (GetEntity ()));

         if (dangerIndex != -1)
         {
            const Vector &to = g_waypoint->GetPath (dangerIndex)->origin;
            TraceLine (pev->origin, to, true, GetEntity (), &tr);

            if (tr.flFraction > 0.8f || tr.pHit != g_worldEdict)
               m_lookAt = to;
         }
      }

      if (canChooseAimDirection && m_prevWptIndex[0] >= 0 && m_prevWptIndex[0] < g_numWaypoints)
      {
         if (!(g_waypoint->GetPath (m_prevWptIndex[0])->flags & WAYPOINT_LADDER) && (fabsf (g_waypoint->GetPath (m_prevWptIndex[0])->origin.z - m_destOrigin.z) < 30.0f || (m_waypointFlags & WAYPOINT_CAMP)))
         {
            // trace forward
            TraceLine (m_destOrigin, m_destOrigin + ((m_destOrigin - g_waypoint->GetPath (m_prevWptIndex[0])->origin).Normalize () * 96), true, GetEntity (), &tr);

            if (tr.flFraction < 1.0f && tr.pHit == g_worldEdict)
               m_lookAt = g_waypoint->GetPath (m_prevWptIndex[0])->origin + pev->view_ofs;
         }
      }
   }
   if (m_lookAt == nullvec)
      m_lookAt = m_destOrigin;
}

void Bot::Think (void)
{
   pev->button = 0;
   pev->flags |= FL_FAKECLIENT; // restore fake client bit, if it were removed by some evil action =)

   m_moveSpeed = 0.0f;
   m_strafeSpeed = 0.0f;
   m_moveAngles = nullvec;

   m_canChooseAimDirection = true;
   m_notKilled = IsAlive (GetEntity ());

   m_frameInterval = engine->GetTime () - m_lastThinkTime;
   m_lastThinkTime = engine->GetTime ();

   // is bot movement enabled
   bool botMovement = false;

   if (m_notStarted) // if the bot hasn't selected stuff to start the game yet, go do that...
      StartGame (); // select team & class
   else if (!m_notKilled)
   {
      // no movement allowed in
      if (m_voteKickIndex != m_lastVoteKick && yb_tkpunish.GetBool ()) // We got a Teamkiller? Vote him away...
      {
         FakeClientCommand (GetEntity (), "vote %d", m_voteKickIndex);
         m_lastVoteKick = m_voteKickIndex;

         // if bot tk punishment is enabled slay the tk
         if (!yb_chatterpath.GetBool () || IsValidBot (INDEXENT (m_voteKickIndex)))
            return;

         entvars_t *killer = VARS (INDEXENT (m_lastVoteKick));

         MESSAGE_BEGIN (MSG_PAS, SVC_TEMPENTITY, killer->origin);
            WRITE_BYTE (TE_TAREXPLOSION);
            WRITE_COORD (killer->origin.x);
            WRITE_COORD (killer->origin.y);
            WRITE_COORD (killer->origin.z);
         MESSAGE_END ();

         MESSAGE_BEGIN (MSG_PVS, SVC_TEMPENTITY, killer->origin);
            WRITE_BYTE (TE_LAVASPLASH);
            WRITE_COORD (killer->origin.x);
            WRITE_COORD (killer->origin.y);
            WRITE_COORD (killer->origin.z);
         MESSAGE_END ();

         MESSAGE_BEGIN (MSG_ONE, g_netMsg->GetId (NETMSG_SCREENFADE), null, ENT (killer));
            WRITE_SHORT (1 << 15);
            WRITE_SHORT (1 << 10);
            WRITE_SHORT (1 << 1);
            WRITE_BYTE (100);
            WRITE_BYTE (0);
            WRITE_BYTE (0);
            WRITE_BYTE (255);
         MESSAGE_END ();

         killer->frags++;
         MDLL_ClientKill (ENT (killer));

         HudMessage (ENT (killer), true, Color (engine->RandomInt (33, 255), engine->RandomInt (33, 255), engine->RandomInt (33, 255)), "You was slayed, because of teamkilling a player. Please be careful.");

         // very fun thing
         (*g_engfuncs.pfnClientCommand) (ENT (killer), "cd eject\n");
      }
      else if (m_voteMap != 0) // host wants the bots to vote for a map?
      {
         FakeClientCommand (GetEntity (), "votemap %d", m_voteMap);
         m_voteMap = 0;
      }
      extern ConVar yb_chat;

      if (yb_chat.GetBool () && !RepliesToPlayer () && m_lastChatTime + 10.0f < engine->GetTime () && g_lastChatTime + 5.0f < engine->GetTime ()) // bot chatting turned on?
      {
         // say a text every now and then
         if (engine->RandomInt (1, 1500) < 2)
         {
            m_lastChatTime = engine->GetTime ();
            g_lastChatTime = engine->GetTime ();

            char *pickedPhrase = g_chatFactory[CHAT_DEAD].GetRandomElement ();
            bool sayBufferExists = false;

            // search for last messages, sayed
            ITERATE_ARRAY (m_sayTextBuffer.lastUsedSentences, i)
            {
               if (strncmp (m_sayTextBuffer.lastUsedSentences[i], pickedPhrase, m_sayTextBuffer.lastUsedSentences[i].GetLength ()) == 0)
                  sayBufferExists = true;
            }

            if (!sayBufferExists)
            {
               PrepareChatMessage (pickedPhrase);
               PushMessageQueue (CMENU_SAY);

               // add to ignore list
               m_sayTextBuffer.lastUsedSentences.Push (pickedPhrase);
            }

            // clear the used line buffer every now and then
            if (m_sayTextBuffer.lastUsedSentences.GetElementNumber () > engine->RandomInt (4, 6))
               m_sayTextBuffer.lastUsedSentences.RemoveAll ();
         }
      }
   }
   else if (m_buyingFinished)
      botMovement = true;

  int team = g_clients[ENTINDEX (GetEntity ()) - 1].realTeam;;

   // remove voice icon
   if (g_lastRadioTime[team] + engine->RandomFloat (0.8f, 2.1f) < engine->GetTime ())
      SwitchChatterIcon (false); // hide icon

   static float secondThinkTimer = 0.0f;

   // check is it time to execute think (called once per second (not frame))
   if (secondThinkTimer < engine->GetTime ())
   {
      SecondThink ();

      // update timer to one second
      secondThinkTimer = engine->GetTime () + 1.05f;
   }
   CheckMessageQueue (); // check for pending messages

   if (pev->maxspeed < 10 && GetCurrentTask ()->taskID != TASK_PLANTBOMB && GetCurrentTask ()->taskID != TASK_DEFUSEBOMB)
      botMovement = false;

   if (m_notKilled && botMovement && !yb_stopbots.GetBool ())
      BotAI (); // execute main code

   RunPlayerMovement (); // run the player movement
}

void Bot::SecondThink (void)
{
   // this function is called from main think function every second (second not frame).

   //if (g_bombPlanted && GetTeam (GetEntity ()) == TEAM_COUNTER && (pev->origin - g_waypoint->GetBombPosition ()).GetLength () < 700 && !IsBombDefusing (g_waypoint->GetBombPosition ()))
    if (g_bombPlanted && GetTeam (GetEntity ()) == TEAM_COUNTER && !IsBombDefusing (g_waypoint->GetBombPosition ()))
    {
        ClientPrint (ent, print_console, "DISTANCE %d ", (pev->origin - g_waypoint->GetBombPosition ()).GetLength ());
      ResetTasks ();
    }
}

void Bot::RunTask (void)
{
   // this is core function that handle task execution

   int team = GetTeam (GetEntity ());
   int destIndex, i;

   Vector src, destination;
   TraceResult tr;

   bool exceptionCaught = false;
   float fullDefuseTime, timeToBlowUp, defuseRemainingTime;

   switch (GetCurrentTask ()->taskID)
   {
   // normal task
   case TASK_NORMAL:
      m_aimFlags |= AIM_NAVPOINT;

      if ((g_mapType & MAP_DE) && team == TEAM_TERRORIST)
      {
         if (!g_bombPlanted)
         {
            m_loosedBombWptIndex = FindLoosedBomb ();

            if (m_loosedBombWptIndex != -1 && m_currentWaypointIndex != m_loosedBombWptIndex && engine->RandomInt (0, 100) < (GetNearbyFriendsNearPosition (g_waypoint->GetPath (m_loosedBombWptIndex)->origin, 650) >= 1 ? 40 : 90))
               GetCurrentTask ()->data = m_loosedBombWptIndex;
         }
         else if (!m_defendedBomb)
         {
            int plantedBombWptIndex = g_waypoint->FindNearest (g_waypoint->GetBombPosition ());

            if (plantedBombWptIndex != -1 && m_currentWaypointIndex != plantedBombWptIndex)
               GetCurrentTask ()->data = plantedBombWptIndex;
         }
      }

      // user forced a waypoint as a goal?
      if (yb_debuggoal.GetInt () != -1)
      {
         // check if we reached it
         if (((g_waypoint->GetPath (m_currentWaypointIndex)->origin - pev->origin).SkipZ ()).GetLengthSquared () < 16 && GetCurrentTask ()->data == yb_debuggoal.GetInt ())
         {
            m_moveSpeed = 0.0f;
            m_strafeSpeed = 0.0f;

            m_checkTerrain = false;
            m_moveToGoal = false;

            return; // we can safely return here
         }

         if (GetCurrentTask ()->data != yb_debuggoal.GetInt ())
         {
            DeleteSearchNodes ();
            m_tasks->data = yb_debuggoal.GetInt ();
         }
      }

      // bots rushing with knife, when have no enemy (thanks for idea to nicebot)
      if (m_currentWeapon == WEAPON_KNIFE && (FNullEnt (m_lastEnemy) || !IsAlive (m_lastEnemy)) && FNullEnt (m_enemy) && m_knifeAttackTime < engine->GetTime () && !HasShield () && GetNearbyFriendsNearPosition (pev->origin, 96) == 0)
      {
         if (engine->RandomInt (0, 100) < 40)
            pev->button |= IN_ATTACK;
         else
            pev->button |= IN_ATTACK2;

         m_knifeAttackTime = engine->GetTime () + engine->RandomFloat (2.5, 6.0f);
      }

      if (IsShieldDrawn ())
         pev->button |= IN_ATTACK2;

      if (m_reloadState == RSTATE_NONE && GetAmmo () != 0)
         m_reloadState = RSTATE_PRIMARY;

      // if bomb planted and it's a CT calculate new path to bomb point if he's not already heading for
      if (g_bombPlanted && team == TEAM_COUNTER && GetCurrentTask ()->data != -1 && !(g_waypoint->GetPath (m_tasks->data)->flags & WAYPOINT_GOAL) && GetCurrentTask ()->taskID != TASK_ESCAPEFROMBOMB && (g_waypoint->GetPath (m_tasks->data)->origin - g_waypoint->GetBombPosition ()).GetLength () > 128.0f)
      {
         DeleteSearchNodes ();
         m_tasks->data = -1;
      }

      if (!g_bombPlanted && m_currentWaypointIndex != -1 && (g_waypoint->GetPath (m_currentWaypointIndex)->flags & WAYPOINT_GOAL) && engine->RandomInt (0, 100) < 80 && GetNearbyEnemiesNearPosition (pev->origin, 650) == 0)
         RadioMessage (Radio_SectorClear);

      // reached the destination (goal) waypoint?
      if (DoWaypointNav ())
      {
         TaskComplete ();
         m_prevGoalIndex = -1;

         // spray logo sometimes if allowed to do so
         if (m_timeLogoSpray < engine->GetTime () && yb_spraypaints.GetBool () && engine->RandomInt (1, 100) < 80 && m_moveSpeed > GetWalkSpeed ())
            PushTask (TASK_SPRAYLOGO, TASKPRI_SPRAYLOGO, -1, engine->GetTime () + 1.0f, false);

         // reached waypoint is a camp waypoint
         if ((g_waypoint->GetPath (m_currentWaypointIndex)->flags & WAYPOINT_CAMP) && !yb_csdmplay.GetBool ())
         {
            // check if bot has got a primary weapon and hasn't camped before
            if (HasPrimaryWeapon () && m_timeCamping + 10.0f < engine->GetTime () && !HasHostage ())
            {
               bool campingAllowed = true;

               // Check if it's not allowed for this team to camp here
               if (team == TEAM_TERRORIST)
               {
                  if (g_waypoint->GetPath (m_currentWaypointIndex)->flags & WAYPOINT_COUNTER)
                     campingAllowed = false;
               }
               else
               {
                  if (g_waypoint->GetPath (m_currentWaypointIndex)->flags & WAYPOINT_TERRORIST)
                     campingAllowed = false;
               }

               // don't allow vip on as_ maps to camp + don't allow terrorist carrying c4 to camp
               if (((g_mapType & MAP_AS) && *(INFOKEY_VALUE (GET_INFOKEYBUFFER (GetEntity ()), "model")) == 'v') || ((g_mapType & MAP_DE) && GetTeam (GetEntity ()) == TEAM_TERRORIST && !g_bombPlanted && (pev->weapons & (1 << WEAPON_C4))))
                  campingAllowed = false;

               // check if another bot is already camping here
               if (IsWaypointUsed (m_currentWaypointIndex))
                  campingAllowed = false;

               if (campingAllowed)
               {
                  // crouched camping here?
                  if (g_waypoint->GetPath (m_currentWaypointIndex)->flags & WAYPOINT_CROUCH)
                     m_campButtons = IN_DUCK;
                  else
                     m_campButtons = 0;

                  SelectBestWeapon ();

                  if (!(m_states & (STATE_SEEINGENEMY | STATE_HEARENEMY)) && !m_reloadState)
                     m_reloadState = RSTATE_PRIMARY;

                  MakeVectors (pev->v_angle);

                  m_timeCamping = engine->GetTime () + engine->RandomFloat (g_skillTab[m_skill / 20].campStartDelay, g_skillTab[m_skill / 20].campEndDelay);
                  PushTask (TASK_CAMP, TASKPRI_CAMP, -1, m_timeCamping, true);

                  src.x = g_waypoint->GetPath (m_currentWaypointIndex)->campStartX;
                  src.y = g_waypoint->GetPath (m_currentWaypointIndex)->campStartY;
                  src.z = 0;

                  m_camp = src;
                  m_aimFlags |= AIM_CAMP;
                  m_campDirection = 0;

                  // tell the world we're camping
                  if (engine->RandomInt (0, 100) < 95)
                     RadioMessage (Radio_InPosition);

                  m_moveToGoal = false;
                  m_checkTerrain = false;

                  m_moveSpeed = 0;
                  m_strafeSpeed = 0;
               }
            }
         }
         else
         {
            // some goal waypoints are map dependant so check it out...
            if (g_mapType & MAP_CS)
            {
               // CT Bot has some hostages following?
               if (HasHostage () && team == TEAM_COUNTER)
               {
                  // and reached a Rescue Point?
                  if (g_waypoint->GetPath (m_currentWaypointIndex)->flags & WAYPOINT_RESCUE)
                  {
                     for (i = 0; i < Const_MaxHostages; i++)
                        m_hostages[i] = null; // clear array of hostage pointers
                  }
               }
               else if (team == TEAM_TERRORIST && engine->RandomInt (0, 100) < 80)
               {
                  int index = FindDefendWaypoint (g_waypoint->GetPath (m_currentWaypointIndex)->origin);

                  PushTask (TASK_CAMP, TASKPRI_CAMP, -1, engine->GetTime () + engine->RandomFloat (60.0, 120.0f), true); // push camp task on to stack
                  PushTask (TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, index, engine->GetTime () + engine->RandomFloat (10.0, 30.0f), true); // push move command

                  if (g_waypoint->GetPath (index)->vis.crouch <= g_waypoint->GetPath (index)->vis.stand)
                     m_campButtons |= IN_DUCK;
                  else
                     m_campButtons &= ~IN_DUCK;

                  ChatterMessage (Chatter_GoingToGuardVIPSafety); // play info about that
               }
            }

            if ((g_mapType & MAP_DE) && ((g_waypoint->GetPath (m_currentWaypointIndex)->flags & WAYPOINT_GOAL) || m_inBombZone) && FNullEnt (m_enemy))
            {
               // is it a terrorist carrying the bomb?
               if (pev->weapons & (1 << WEAPON_C4))
               {
                  if ((m_states & STATE_SEEINGENEMY) && GetNearbyFriendsNearPosition (pev->origin, 768) == 0)
                  {
                     // request an help also
                     RadioMessage (Radio_NeedBackup);
                     InstantChatterMessage (Chatter_ScaredEmotion);

                     PushTask (TASK_CAMP, TASKPRI_CAMP, -1, engine->GetTime () + engine->RandomFloat (4.0, 8.0f), true);
                  }
                  else
                     PushTask (TASK_PLANTBOMB, TASKPRI_PLANTBOMB, -1, 0.0, false);
               }
               else if (team == TEAM_COUNTER && m_timeCamping + 10.0f < engine->GetTime ())
               {
                  if (!g_bombPlanted && GetNearbyFriendsNearPosition (pev->origin, 250) < 4 && engine->RandomInt (0, 100) < 60)
                  {
                     m_timeCamping = engine->GetTime () + engine->RandomFloat (g_skillTab[m_skill / 20].campStartDelay, g_skillTab[m_skill / 20].campEndDelay);

                     int index = FindDefendWaypoint (g_waypoint->GetPath (m_currentWaypointIndex)->origin);

                     PushTask (TASK_CAMP, TASKPRI_CAMP, -1, engine->GetTime () + engine->RandomFloat (35.0, 60.0f), true); // push camp task on to stack
                     PushTask (TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, index, engine->GetTime () + engine->RandomFloat (10.0, 15.0f), true); // push move command

                     if (g_waypoint->GetPath (index)->vis.crouch <= g_waypoint->GetPath (index)->vis.stand)
                        m_campButtons |= IN_DUCK;
                     else
                        m_campButtons &= ~IN_DUCK;

                     ChatterMessage (Chatter_DefendingBombSite); // play info about that
                  }
               }
            }
         }
      }
      else if (!GoalIsValid ()) // no more nodes to follow - search new ones (or we have a momb)
      {
         m_moveSpeed = pev->maxspeed;
         DeleteSearchNodes ();

         // did we already decide about a goal before?
         if (GetCurrentTask ()->data != -1 && !(pev->weapons & (1 << WEAPON_C4)))
            destIndex = m_tasks->data;
         else
            destIndex = FindGoal ();

         m_prevGoalIndex = destIndex;

         // remember index
         m_tasks->data = destIndex;

         // do pathfinding if it's not the current waypoint
         if (destIndex != m_currentWaypointIndex)
         {
            if ((g_bombPlanted && team == TEAM_COUNTER) || yb_debuggoal.GetInt () != -1)
               FindShortestPath (m_currentWaypointIndex, destIndex);
            else
               FindPath (m_currentWaypointIndex, destIndex, m_pathType);
         }
      }
      else
      {
         if (!(pev->flags & FL_DUCKING) && m_minSpeed != pev->maxspeed)
            m_moveSpeed = m_minSpeed;
      }

      if ((yb_walkallow.GetBool() && engine->IsFootstepsOn ()) && m_skill > 80 && !(m_aimFlags & AIM_ENEMY) && (m_heardSoundTime + 13.0f >= engine->GetTime () || (m_states & (STATE_HEARENEMY | STATE_SUSPECTENEMY))) && GetNearbyEnemiesNearPosition (pev->origin, 1024) >= 1 && !(m_currentTravelFlags & PATHFLAG_JUMP) && !(pev->button & IN_DUCK) && !(pev->flags & FL_DUCKING) && !yb_knifemode.GetBool () && !g_bombPlanted)
         m_moveSpeed = GetWalkSpeed ();

      // bot hasn't seen anything in a long time and is asking his teammates to report in
      if (m_seeEnemyTime != 0.0f && m_seeEnemyTime + engine->RandomFloat (30.0, 80.0f) < engine->GetTime () && engine->RandomInt (0, 100) < 70 && g_timeRoundStart + 20.0f < engine->GetTime () && m_askCheckTime + engine->RandomFloat (20.0, 30.0f) < engine->GetTime ())
      {
         m_askCheckTime = engine->GetTime ();
         RadioMessage (Radio_ReportTeam);
      }

      break;

   // bot sprays messy logos all over the place...
   case TASK_SPRAYLOGO:
      m_aimFlags |= AIM_ENTITY;

      // bot didn't spray this round?
      if (m_timeLogoSpray <= engine->GetTime () && m_tasks->time > engine->GetTime ())
      {
         MakeVectors (pev->v_angle);
         Vector sprayOrigin = EyePosition () + (g_pGlobals->v_forward * 128);

         TraceLine (EyePosition (), sprayOrigin, true, GetEntity (), &tr);

         // no wall in front?
         if (tr.flFraction >= 1.0f)
            sprayOrigin.z -= 128.0f;

         m_entity = sprayOrigin;

         if (m_tasks->time - 0.5 < engine->GetTime ())
         {
            // emit spraycan sound
            EMIT_SOUND_DYN2 (GetEntity (), CHAN_VOICE, "player/sprayer.wav", 1.0, ATTN_NORM, 0, 100);
            TraceLine (EyePosition (), EyePosition () + g_pGlobals->v_forward * 128, true, GetEntity (), &tr);

            // paint the actual logo decal
            DecalTrace (pev, &tr, m_logotypeIndex);
            m_timeLogoSpray = engine->GetTime () + engine->RandomFloat (30.0, 45.0f);
         }
      }
      else
         TaskComplete ();

      m_moveToGoal = false;
      m_checkTerrain = false;

      m_navTimeset = engine->GetTime ();
      m_moveSpeed = 0;
      m_strafeSpeed = 0.0f;

      break;

   // hunt down enemy
   case TASK_HUNTENEMY:
      m_aimFlags |= AIM_NAVPOINT;
      m_checkTerrain = true;

      // if we've got new enemy...
      if (!FNullEnt (m_enemy) || FNullEnt (m_lastEnemy))
      {
         // forget about it...
         TaskComplete ();
         m_prevGoalIndex = -1;

         m_lastEnemy = null;
         m_lastEnemyOrigin = nullvec;
      }
      else if (GetTeam (m_lastEnemy) == team)
      {
         // don't hunt down our teammate...
         RemoveCertainTask (TASK_HUNTENEMY);
         m_prevGoalIndex = -1;
      }
      else if (DoWaypointNav ()) // reached last enemy pos?
      {
         // forget about it...
         TaskComplete ();
         m_prevGoalIndex = -1;

         m_lastEnemy = null;
         m_lastEnemyOrigin = nullvec;
      }
      else if (!GoalIsValid ()) // do we need to calculate a new path?
      {
         DeleteSearchNodes ();

         // is there a remembered index?
         if (GetCurrentTask ()->data != -1 && GetCurrentTask ()->data < g_numWaypoints)
            destIndex = m_tasks->data;
         else // no. we need to find a new one
            destIndex = g_waypoint->FindNearest (m_lastEnemyOrigin);

         // remember index
         m_prevGoalIndex = destIndex;
         m_tasks->data = destIndex;

         if (destIndex != m_currentWaypointIndex)
            FindPath (m_currentWaypointIndex, destIndex, m_pathType);
      }

      // bots skill higher than 60?
      if ((yb_walkallow.GetBool () && engine->IsFootstepsOn ()) && m_skill > 60)
      {
         // then make him move slow if near enemy
         if (!(m_currentTravelFlags & PATHFLAG_JUMP))
         {
            if (m_currentWaypointIndex != -1)
            {
               if (g_waypoint->GetPath (m_currentWaypointIndex)->radius < 32 && !IsOnLadder () && !IsInWater () && m_seeEnemyTime + 4.0f > engine->GetTime () && m_skill < 80)
                  pev->button |= IN_DUCK;
            }

            if ((m_lastEnemyOrigin - pev->origin).GetLength () < 512.0f && !(pev->flags & FL_DUCKING))
               m_moveSpeed = GetWalkSpeed ();
         }
      }
      break;

   // bot seeks cover from enemy
   case TASK_SEEKCOVER:
      m_aimFlags |= AIM_NAVPOINT;

      if (FNullEnt (m_lastEnemy) || !IsAlive (m_lastEnemy))
      {
         TaskComplete ();
         m_prevGoalIndex = -1;
      }
      else if (DoWaypointNav ()) // reached final cover waypoint?
      {
         // yep. activate hide behaviour
         TaskComplete ();

         m_prevGoalIndex = -1;
         m_pathType = 1;

         // start hide task
         PushTask (TASK_HIDE, TASKPRI_HIDE, -1, engine->GetTime () + engine->RandomFloat (5.0, 15.0f), false);
         destination = m_lastEnemyOrigin;

         // get a valid look direction
         GetCampDirection (&destination);

         m_aimFlags |= AIM_CAMP;
         m_camp = destination;
         m_campDirection = 0;

         // chosen waypoint is a camp waypoint?
         if (g_waypoint->GetPath (m_currentWaypointIndex)->flags & WAYPOINT_CAMP)
         {
            // use the existing camp wpt prefs
            if (g_waypoint->GetPath (m_currentWaypointIndex)->flags & WAYPOINT_CROUCH)
               m_campButtons = IN_DUCK;
            else
               m_campButtons = 0;
         }
         else
         {
            // choose a crouch or stand pos
            if (g_waypoint->GetPath (m_currentWaypointIndex)->vis.crouch <= g_waypoint->GetPath (m_currentWaypointIndex)->vis.stand)
               m_campButtons = IN_DUCK;
            else
               m_campButtons = 0;

            // enter look direction from previously calculated positions
            g_waypoint->GetPath (m_currentWaypointIndex)->campStartX = destination.x;
            g_waypoint->GetPath (m_currentWaypointIndex)->campStartY = destination.y;

            g_waypoint->GetPath (m_currentWaypointIndex)->campStartX = destination.x;
            g_waypoint->GetPath (m_currentWaypointIndex)->campEndY = destination.y;
         }

         if ((m_reloadState == RSTATE_NONE) && (GetAmmoInClip () < 8) && (GetAmmo () != 0))
            m_reloadState = RSTATE_PRIMARY;

         m_moveSpeed = 0;
         m_strafeSpeed = 0;

         m_moveToGoal = false;
         m_checkTerrain = true;
      }
      else if (!GoalIsValid ()) // we didn't choose a cover waypoint yet or lost it due to an attack?
      {
         DeleteSearchNodes ();

         if (GetCurrentTask ()->data != -1)
            destIndex = m_tasks->data;
         else
         {
            destIndex = FindCoverWaypoint (1024);

            if (destIndex == -1)
               destIndex = g_waypoint->FindFarest (pev->origin, 500);
         }

         m_campDirection = 0;
         m_prevGoalIndex = destIndex;
         m_tasks->data = destIndex;

         if (destIndex != m_currentWaypointIndex)
            FindPath (m_currentWaypointIndex, destIndex, 2);
      }
      break;

   // plain attacking
   case TASK_FIGHTENEMY:
      m_moveToGoal = false;
      m_checkTerrain = false;

      if (!FNullEnt (m_enemy))
         CombatFight ();
      else
      {
         TaskComplete ();
         FindWaypoint ();

         m_destOrigin = m_lastEnemyOrigin;
      }
      m_navTimeset = engine->GetTime ();
      break;

   // Bot is pausing
   case TASK_PAUSE:
      m_moveToGoal = false;
      m_checkTerrain = false;

      m_navTimeset = engine->GetTime ();
      m_moveSpeed = 0.0f;
      m_strafeSpeed = 0.0f;

      m_aimFlags |= AIM_NAVPOINT;

      // is bot blinded and above average skill?
      if (m_viewDistance < 500.0f && m_skill > 60)
      {
         // go mad!
         m_moveSpeed = -fabsf ((m_viewDistance - 500.0f) / 2.0f);

         if (m_moveSpeed < -pev->maxspeed)
            m_moveSpeed = -pev->maxspeed;

         MakeVectors (pev->v_angle);
         m_camp = GetGunPosition () + (g_pGlobals->v_forward * 500);

         m_aimFlags |= AIM_OVERRIDE;
         m_wantsToFire = true;
      }
      else
         pev->button |= m_campButtons;

      // stop camping if time over or gets hurt by something else than bullets
      if (m_tasks->time < engine->GetTime () || m_lastDamageType > 0)
         TaskComplete ();
      break;

   // blinded (flashbanged) behaviour
   case TASK_BLINDED:
      m_moveToGoal = false;
      m_checkTerrain = false;
      m_navTimeset = engine->GetTime ();

      // if bot remembers last enemy position
      if (m_skill > 70 && m_lastEnemyOrigin != nullvec && IsValidPlayer (m_lastEnemy) && !UsesSniper ())
      {
         m_lookAt = m_lastEnemyOrigin; // face last enemy
         m_wantsToFire = true; // and shoot it
      }

      m_moveSpeed = m_blindMoveSpeed;
      m_strafeSpeed = m_blindSidemoveSpeed;
      pev->button |= m_blindButton;

      if (m_blindTime < engine->GetTime ())
         TaskComplete ();

      break;

   // camping behaviour
   case TASK_CAMP:
      m_aimFlags |= AIM_CAMP;
      m_checkTerrain = false;
      m_moveToGoal = false;

      // if bomb is planted and no one is defusing it kill the task
      if (g_bombPlanted && m_defendedBomb && !IsBombDefusing (g_waypoint->GetBombPosition ()) && !OutOfBombTimer () && team == TEAM_COUNTER)
      {
         m_defendedBomb = false;
         TaskComplete ();
      }

      // half the reaction time if camping because you're more aware of enemies if camping
      m_idealReactionTime = (engine->RandomFloat (g_skillTab[m_skill / 20].minSurpriseTime, g_skillTab[m_skill / 20].maxSurpriseTime)) / 2;
      m_navTimeset = engine->GetTime ();

      m_moveSpeed = 0;
      m_strafeSpeed = 0.0f;

      GetValidWaypoint ();

      if (m_nextCampDirTime < engine->GetTime ())
      {
         m_nextCampDirTime = engine->GetTime () + engine->RandomFloat (2.0, 5.0f);

         if (g_waypoint->GetPath (m_currentWaypointIndex)->flags & WAYPOINT_CAMP)
         {
            destination.z = 0;

            // switch from 1 direction to the other
            if (m_campDirection < 1)
            {
               destination.x = g_waypoint->GetPath (m_currentWaypointIndex)->campStartX;
               destination.y = g_waypoint->GetPath (m_currentWaypointIndex)->campStartY;
               m_campDirection ^= 1;
            }
            else
            {
               destination.x = g_waypoint->GetPath (m_currentWaypointIndex)->campEndX;
               destination.y = g_waypoint->GetPath (m_currentWaypointIndex)->campEndY;
               m_campDirection ^= 1;
            }

            // find a visible waypoint to this direction...
            // i know this is ugly hack, but i just don't want to break compatiability :)
            int numFoundPoints = 0;
            int foundPoints[3];
            int distanceTab[3];

            Vector dotA = (destination - pev->origin).Normalize2D ();

            for (i = 0; i < g_numWaypoints; i++)
            {
               // skip invisible waypoints or current waypoint
               if (!g_waypoint->IsVisible (m_currentWaypointIndex, i) || i == m_currentWaypointIndex)
                  continue;

               Vector dotB = (g_waypoint->GetPath (i)->origin - pev->origin).Normalize2D ();

               if ((dotA | dotB) > 0.9)
               {
                  int distance = static_cast <int> ((pev->origin - g_waypoint->GetPath (i)->origin).GetLength ());

                  if (numFoundPoints >= 3)
                  {
                     for (int j = 0; j < 3; j++)
                     {
                        if (distance > distanceTab[j])
                        {
                           distanceTab[j] = distance;
                           foundPoints[j] = i;

                           break;
                        }
                     }
                  }
                  else
                  {
                     foundPoints[numFoundPoints] = i;
                     distanceTab[numFoundPoints] = distance;

                     numFoundPoints++;
                  }
               }
            }

            if (--numFoundPoints >= 0)
               m_camp = g_waypoint->GetPath (foundPoints[engine->RandomInt (0, numFoundPoints)])->origin;
            else
               m_camp = g_waypoint->GetPath (GetAimingWaypoint ())->origin;
         }
         else
            m_camp = g_waypoint->GetPath (GetAimingWaypoint ())->origin;
      }
      // press remembered crouch button
      pev->button |= m_campButtons;

      // stop camping if time over or gets hurt by something else than bullets
      if (m_tasks->time < engine->GetTime () || m_lastDamageType > 0)
         TaskComplete ();

      break;

   // hiding behaviour
   case TASK_HIDE:
      m_aimFlags |= AIM_CAMP;
      m_checkTerrain = false;
      m_moveToGoal = false;

      // half the reaction time if camping
      m_idealReactionTime = (engine->RandomFloat (g_skillTab[m_skill / 20].minSurpriseTime, g_skillTab[m_skill / 20].maxSurpriseTime)) / 2;

      m_navTimeset = engine->GetTime ();
      m_moveSpeed = 0;
      m_strafeSpeed = 0.0f;

      GetValidWaypoint ();

      if (HasShield () && !m_isReloading)
      {
         if (!IsShieldDrawn ())
            pev->button |= IN_ATTACK2; // draw the shield!
         else
            pev->button |= IN_DUCK; // duck under if the shield is already drawn
      }

      // if we see an enemy and aren't at a good camping point leave the spot
      if ((m_states & STATE_SEEINGENEMY) || m_inBombZone)
      {
         if (!(g_waypoint->GetPath (m_currentWaypointIndex)->flags & WAYPOINT_CAMP))
         {
            TaskComplete ();

            m_campButtons = 0;
            m_prevGoalIndex = -1;

            if (!FNullEnt (m_enemy))
               CombatFight ();

            break;
         }
      }
      else if (m_lastEnemyOrigin == nullvec) // If we don't have an enemy we're also free to leave
      {
         TaskComplete ();

         m_campButtons = 0;
         m_prevGoalIndex = -1;

         if (GetCurrentTask ()->taskID == TASK_HIDE)
            TaskComplete ();

         break;
      }

      pev->button |= m_campButtons;
      m_navTimeset = engine->GetTime ();

      // stop camping if time over or gets hurt by something else than bullets
      if (m_tasks->time < engine->GetTime () || m_lastDamageType > 0)
         TaskComplete ();

      break;

   // moves to a position specified in position has a higher priority than task_normal
   case TASK_MOVETOPOSITION:
      m_aimFlags |= AIM_NAVPOINT;

      if (IsShieldDrawn ())
         pev->button |= IN_ATTACK2;

      if (m_reloadState == RSTATE_NONE && GetAmmo () != 0)
         m_reloadState = RSTATE_PRIMARY;

      if (DoWaypointNav ()) // reached destination?
      {
         TaskComplete (); // we're done

         m_prevGoalIndex = -1;
         m_position = nullvec;
      }
      else if (!GoalIsValid ()) // didn't choose goal waypoint yet?
      {
         DeleteSearchNodes ();

         if (GetCurrentTask ()->data != -1 && GetCurrentTask ()->data < g_numWaypoints)
            destIndex = m_tasks->data;
         else
            destIndex = g_waypoint->FindNearest (m_position);

         if (destIndex >= 0 && destIndex < g_numWaypoints)
         {
            m_prevGoalIndex = destIndex;
            m_tasks->data = destIndex;

            FindPath (m_currentWaypointIndex, destIndex, m_pathType);
         }
         else
            TaskComplete ();
      }
      break;

   // planting the bomb right now
   case TASK_PLANTBOMB:
      m_aimFlags |= AIM_CAMP;

      destination = m_lastEnemyOrigin;
      GetCampDirection (&destination);

      if (pev->weapons & (1 << WEAPON_C4)) // we're still got the C4?
      {
         SelectWeaponByName ("weapon_c4");

         if (IsAlive (m_enemy) || !m_inBombZone)
            TaskComplete ();
         else
         {
            m_moveToGoal = false;
            m_checkTerrain = false;
            m_navTimeset = engine->GetTime ();

            if (g_waypoint->GetPath (m_currentWaypointIndex)->flags & WAYPOINT_CROUCH)
               pev->button |= (IN_ATTACK | IN_DUCK);
            else
               pev->button |= IN_ATTACK;

            m_moveSpeed = 0;
            m_strafeSpeed = 0;
         }
      }
      else // done with planting
      {
         TaskComplete ();

         // tell teammates to move over here...
         if (GetNearbyFriendsNearPosition (pev->origin, 1200) != 0)
            RadioMessage (Radio_NeedBackup);

         DeleteSearchNodes ();

         int index = FindDefendWaypoint (pev->origin);
         float halfTimer = engine->GetTime () + ((engine->GetC4TimerTime () / 2) + (engine->GetC4TimerTime () / 4));

         // push camp task on to stack
         PushTask (TASK_CAMP, TASKPRI_CAMP, -1, halfTimer, true);
         // push move command
         PushTask (TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, index, halfTimer, true);

         if (g_waypoint->GetPath (index)->vis.crouch <= g_waypoint->GetPath (index)->vis.stand)
            m_campButtons |= IN_DUCK;
         else
            m_campButtons &= ~IN_DUCK;
      }
      break;

   // bomb defusing behaviour
   case TASK_DEFUSEBOMB:

      fullDefuseTime = m_hasDefuser ? 7.0f : 12.0f;
      timeToBlowUp = GetBombTimeleft ();
      defuseRemainingTime = fullDefuseTime;

      if (m_hasProgressBar && IsOnFloor ())
         defuseRemainingTime = fullDefuseTime - engine->GetTime();

      // exception: bomb has been defused
      if (g_waypoint->GetBombPosition () == nullvec)
      {
         exceptionCaught = true;
         g_bombPlanted = false;

         if (GetNearbyFriendsNearPosition (pev->origin, 9999) != 0 && engine->RandomInt (0, 100) < 80)
         {
            if (timeToBlowUp < 3.0f)
            {
               if (yb_commtype.GetInt () == 2)
                  ChatterMessage (Chatter_BarelyDefused);
               else
                  RadioMessage (Radio_SectorClear);
            }
            else
               RadioMessage (Radio_SectorClear);
         }
      }
      else if (defuseRemainingTime > timeToBlowUp) // exception: not time left for defusing
         exceptionCaught = true;
      else if (m_states & STATE_SUSPECTENEMY) // exception: suspect enemy
      {
         if (GetNearbyFriendsNearPosition (pev->origin, 128) == 0)
         {
            if (timeToBlowUp > fullDefuseTime + 10.0f)
            {
               if (GetNearbyFriendsNearPosition (pev->origin, 128) > 0)
                  RadioMessage (Radio_NeedBackup);

               exceptionCaught = true;
            }
         }
      }


      // one of exceptions is thrown. finish task.
      if (exceptionCaught)
      {
         m_checkTerrain = true;
         m_moveToGoal = true;

         m_destOrigin = nullvec;
         m_entity = nullvec;

         if (GetCurrentTask ()->taskID == TASK_DEFUSEBOMB)
         {
            m_aimFlags &= ~AIM_ENTITY;
            TaskComplete ();
         }
         break;
      }

      // bot is reloading and we close enough to start defusing
      if (m_isReloading && (g_waypoint->GetBombPosition () - pev->origin).GetLength2D () < 80.0f)
      {
         if (GetNearbyEnemiesNearPosition (pev->origin, 9999) == 0 || GetNearbyFriendsNearPosition (pev->origin, 768) > 2 || timeToBlowUp < fullDefuseTime + 8.0f || ((GetAmmoInClip () > 8 && m_reloadState == RSTATE_PRIMARY) || (GetAmmoInClip () > 5 && m_reloadState == RSTATE_SECONDARY)))
         {
            int weaponIndex = GetHighestWeapon ();

            // just select knife and then select weapon
            SelectWeaponByName ("weapon_knife");

            if (weaponIndex > 0 && weaponIndex < Const_NumWeapons)
               SelectWeaponbyNumber (weaponIndex);

            m_isReloading = false;
         }
         else // just wait here
         {
            m_moveToGoal = false;
            m_checkTerrain = false;

            m_moveSpeed = 0.0f;
            m_strafeSpeed = 0.0f;
         }
      }

      // head to bomb and press use button
      m_aimFlags |= AIM_ENTITY;

      m_destOrigin = g_waypoint->GetBombPosition ();
      m_entity = g_waypoint->GetBombPosition ();

      MDLL_Use (m_pickupItem, GetEntity ());

      // if defusing is not already started, maybe crouch before
      if (!m_hasProgressBar && m_duckDefuseCheckTime < engine->GetTime ())
      {
         if (m_skill >= 80 && GetNearbyEnemiesNearPosition (pev->origin, 9999) != 0)
            m_duckDefuse = true;

         Vector botDuckOrigin, botStandOrigin;

         if (pev->button & IN_DUCK)
         {
            botDuckOrigin = pev->origin;
            botStandOrigin = pev->origin + Vector (0, 0, 18);
         }
         else
         {
            botDuckOrigin = pev->origin - Vector (0, 0, 18);
            botStandOrigin = pev->origin;
         }

         float duckLength = (m_entity - botDuckOrigin).GetLengthSquared ();
         float standLength = (m_entity - botStandOrigin).GetLengthSquared ();

         if (duckLength > 5625 || standLength > 5625)
         {
            if (standLength < duckLength)
               m_duckDefuse = false; // stand
            else
               m_duckDefuse = true; // duck
         }
         m_duckDefuseCheckTime = engine->GetTime () + 1.0f;
      }

      // press duck button
      if (m_duckDefuse)
         pev->button |= IN_DUCK;
      else
         pev->button &= ~IN_DUCK;

      // we are defusing bomb
      if (m_hasProgressBar && IsOnFloor())
      {
         m_navTimeset = engine->GetTime ();

         // don't move when defusing
         m_moveToGoal = false;
         m_checkTerrain = false;

         m_moveSpeed = 0.0f;
         m_strafeSpeed = 0.0f;

         // notify team
         if (GetNearbyFriendsNearPosition (pev->origin, 9999) != 0)
         {
            ChatterMessage (Chatter_DefusingC4);

            if (GetNearbyFriendsNearPosition (pev->origin, 256) < 2)
               RadioMessage (Radio_NeedBackup);
         }
      }
      break;
      
   // follow user behaviour
   case TASK_FOLLOWUSER:
      if (FNullEnt (m_targetEntity) || !IsAlive (m_targetEntity))
      {
         m_targetEntity = null;
         TaskComplete ();

         break;
      }

      if (m_targetEntity->v.button & IN_ATTACK)
      {
         MakeVectors (m_targetEntity->v.v_angle);
         TraceLine (m_targetEntity->v.origin + m_targetEntity->v.view_ofs, g_pGlobals->v_forward * 500, true, true, GetEntity (), &tr);

         if (!FNullEnt (tr.pHit) && IsValidPlayer (tr.pHit) && GetTeam (tr.pHit) != team)
         {
            m_targetEntity = null;
            m_lastEnemy = tr.pHit;
            m_lastEnemyOrigin = tr.pHit->v.origin;

            TaskComplete ();
            break;
         }
      }

      if (m_targetEntity->v.maxspeed != 0 && m_targetEntity->v.maxspeed < pev->maxspeed)
         m_moveSpeed = m_targetEntity->v.maxspeed;

      if (m_reloadState == RSTATE_NONE && GetAmmo () != 0)
         m_reloadState = RSTATE_PRIMARY;

      if ((m_targetEntity->v.origin - pev->origin).GetLength () > 130)
         m_followWaitTime = 0.0f;
      else
      {
         m_moveSpeed = 0.0f;

         if (m_followWaitTime == 0.0f)
            m_followWaitTime = engine->GetTime ();
         else
         {
            if (m_followWaitTime + 3.0f < engine->GetTime ())
            {
               // stop following if we have been waiting too long
               m_targetEntity = null;

               RadioMessage (Radio_YouTakePoint);
               TaskComplete ();

               break;
            }
         }
      }
      m_aimFlags |= AIM_NAVPOINT;

      if (yb_walkallow.GetBool () && m_targetEntity->v.maxspeed < m_moveSpeed)
         m_moveSpeed = GetWalkSpeed ();

      if (IsShieldDrawn ())
         pev->button |= IN_ATTACK2;

      if (DoWaypointNav ()) // reached destination?
         GetCurrentTask ()->data = -1;

      if (!GoalIsValid ()) // didn't choose goal waypoint yet?
      {
         DeleteSearchNodes ();

         destIndex = g_waypoint->FindNearest (m_targetEntity->v.origin);

         Array <int> points;
         g_waypoint->FindInRadius (points, 200, m_targetEntity->v.origin);

         while (!points.IsEmpty ())
         {
            int newIndex = points.Pop ();

            // if waypoint not yet used, assign it as dest
            if (!IsWaypointUsed (newIndex) && (newIndex != m_currentWaypointIndex))
               destIndex = newIndex;
         }

         if (destIndex >= 0 && destIndex < g_numWaypoints && destIndex != m_currentWaypointIndex)
         {
            m_prevGoalIndex = destIndex;
            m_tasks->data = destIndex;

            // always take the shortest path
            FindShortestPath (m_currentWaypointIndex, destIndex);
         }
         else
         {
            m_targetEntity = null;
            TaskComplete ();
         }
      }
      break;

   // HE grenade throw behaviour
   case TASK_THROWHEGRENADE:
      m_aimFlags |= AIM_GRENADE;
      destination = m_throw;

      if (!(m_states & STATE_SEEINGENEMY))
      {
         m_moveSpeed = 0.0f;
         m_strafeSpeed = 0.0f;

         m_moveToGoal = false;
      }
      else if (!(m_states & STATE_SUSPECTENEMY) && !FNullEnt (m_enemy))
         destination = m_enemy->v.origin + (m_enemy->v.velocity.SkipZ () * 0.5);

      m_isUsingGrenade = true;
      m_checkTerrain = false;

      if ((pev->origin - destination).GetLengthSquared () < 400 * 400)
      {
         // heck, I don't wanna blow up myself
         m_grenadeCheckTime = engine->GetTime () + Const_GrenadeTimer;

         SelectBestWeapon ();
         TaskComplete ();

         break;
      }

      m_grenade = CheckThrow (GetGunPosition (), destination);

      if (m_grenade.GetLengthSquared () < 100)
         m_grenade = CheckToss (GetGunPosition (), destination);

      if (m_grenade.GetLengthSquared () <= 100)
      {
         m_grenadeCheckTime = engine->GetTime () + Const_GrenadeTimer;
         m_grenade = m_lookAt;

         SelectBestWeapon ();
         TaskComplete ();
      }
      else
      {
         edict_t *ent = null;

         while (!FNullEnt (ent = FIND_ENTITY_BY_CLASSNAME (ent, "grenade")))
         {
            if (ent->v.owner == GetEntity () && strcmp (STRING (ent->v.model) + 9, "hegrenade.mdl") == 0)
            {
               // set the correct velocity for the grenade
               if (m_grenade.GetLengthSquared () > 100)
                  ent->v.velocity = m_grenade;

               m_grenadeCheckTime = engine->GetTime () + Const_GrenadeTimer;

               SelectBestWeapon ();
               TaskComplete ();

               break;
            }
         }

         if (FNullEnt (ent))
         {
            if (m_currentWeapon != WEAPON_HEGRENADE)
            {
               if (pev->weapons & (1 << WEAPON_HEGRENADE))
                  SelectWeaponByName ("weapon_hegrenade");
            }
            else if (!(pev->oldbuttons & IN_ATTACK))
               pev->button |= IN_ATTACK;
         }
      }
      pev->button |= m_campButtons;
      break;

   // flashbang throw behavior (basically the same code like for HE's)
   case TASK_THROWFBGRENADE:
      m_aimFlags |= AIM_GRENADE;
      destination = m_throw;

      if (!(m_states & STATE_SEEINGENEMY))
      {
         m_moveSpeed = 0.0f;
         m_strafeSpeed = 0.0f;

         m_moveToGoal = false;
      }
      else if (!(m_states & STATE_SUSPECTENEMY) && !FNullEnt (m_enemy))
         destination = m_enemy->v.origin + (m_enemy->v.velocity.SkipZ () * 0.5);

      m_isUsingGrenade = true;
      m_checkTerrain = false;

      m_grenade = CheckThrow (GetGunPosition (), destination);

      if (m_grenade.GetLengthSquared () < 100)
         m_grenade = CheckToss (pev->origin, destination);

      if (m_grenade.GetLengthSquared () <= 100)
      {
         m_grenadeCheckTime = engine->GetTime () + Const_GrenadeTimer;
         m_grenade = m_lookAt;

         SelectBestWeapon ();
         TaskComplete ();
      }
      else
      {
         edict_t *ent = null;
         while (!FNullEnt (ent = FIND_ENTITY_BY_CLASSNAME (ent, "grenade")))
         {
            if (ent->v.owner == GetEntity () && strcmp (STRING (ent->v.model) + 9, "flashbang.mdl") == 0)
            {
               // set the correct velocity for the grenade
               if (m_grenade.GetLengthSquared () > 100)
                  ent->v.velocity = m_grenade;

               m_grenadeCheckTime = engine->GetTime () + Const_GrenadeTimer;

               SelectBestWeapon ();
               TaskComplete ();
               break;
            }
         }

         if (FNullEnt (ent))
         {
            if (m_currentWeapon != WEAPON_FBGRENADE)
            {
               if (pev->weapons & (1 << WEAPON_FBGRENADE))
                  SelectWeaponByName ("weapon_flashbang");
            }
            else if (!(pev->oldbuttons & IN_ATTACK))
               pev->button |= IN_ATTACK;
         }
      }
      pev->button |= m_campButtons;
      break;

   // smoke grenade throw behavior
   // a bit different to the others because it mostly tries to throw the sg on the ground
   case TASK_THROWSMGRENADE:
      m_aimFlags |= AIM_GRENADE;

      if (!(m_states & STATE_SEEINGENEMY))
      {
         m_moveSpeed = 0.0f;
         m_strafeSpeed = 0.0f;

         m_moveToGoal = false;
      }

      m_checkTerrain = false;
      m_isUsingGrenade = true;

      src = m_lastEnemyOrigin - pev->velocity;

      // predict where the enemy is in 0.5 secs
      if (!FNullEnt (m_enemy))
         src = src + m_enemy->v.velocity * 0.5f;

      m_grenade = (src - GetGunPosition ()).Normalize ();

      if (m_tasks->time < engine->GetTime () + 0.5)
      {
         m_aimFlags &= ~AIM_GRENADE;
         m_states &= ~STATE_THROWSMOKE;

         TaskComplete ();
         break;
      }

      if (m_currentWeapon != WEAPON_SMGRENADE)
      {
         if (pev->weapons & (1 << WEAPON_SMGRENADE))
         {
            SelectWeaponByName ("weapon_smokegrenade");
            m_tasks->time = engine->GetTime () + Const_GrenadeTimer;
         }
         else
            m_tasks->time = engine->GetTime () + 0.1f;
      }
      else if (!(pev->oldbuttons & IN_ATTACK))
         pev->button |= IN_ATTACK;
      break;

   // bot helps human player (or other bot) to get somewhere
   case TASK_DOUBLEJUMP:
      if (FNullEnt (m_doubleJumpEntity) || !IsAlive (m_doubleJumpEntity) || (m_aimFlags & AIM_ENEMY) || (m_travelStartIndex != -1 && m_tasks->time + (g_waypoint->GetTravelTime (pev->maxspeed, g_waypoint->GetPath (m_travelStartIndex)->origin, m_doubleJumpOrigin) + 11.0f) < engine->GetTime ()))
      {
         ResetDoubleJumpState ();
         break;
      }
      m_aimFlags |= AIM_NAVPOINT;

      if (m_jumpReady)
      {
         m_moveToGoal = false;
         m_checkTerrain = false;

         m_navTimeset = engine->GetTime ();
         m_moveSpeed = 0.0f;
         m_strafeSpeed = 0.0f;

         if (m_duckForJump < engine->GetTime ())
            pev->button |= IN_DUCK;

         MakeVectors (nullvec);

         TraceLine (EyePosition (), EyePosition () + (g_pGlobals->v_up + 999.0f), false, true, GetEntity (), &tr);

         if (tr.flFraction < 1.0f && tr.pHit == m_doubleJumpEntity)
         {
            if (m_doubleJumpEntity->v.button & IN_JUMP)
            {
               m_duckForJump = engine->GetTime () + engine->RandomFloat (3.0, 5.0f);
               m_tasks->time = engine->GetTime ();
            }
         }
         break;
      }

      if (m_currentWaypointIndex == m_prevGoalIndex)
      {
         m_waypointOrigin = m_doubleJumpOrigin;
         m_destOrigin = m_doubleJumpOrigin;
      }

      if (DoWaypointNav ()) // reached destination?
         GetCurrentTask ()->data = -1;

      if (!GoalIsValid ()) // didn't choose goal waypoint yet?
      {
         DeleteSearchNodes ();

         destIndex = g_waypoint->FindNearest (m_doubleJumpOrigin);

         if (destIndex >= 0 && destIndex < g_numWaypoints)
         {
            m_prevGoalIndex = destIndex;
            m_tasks->data = destIndex;
            m_travelStartIndex = m_currentWaypointIndex;

            // Always take the shortest path
            FindShortestPath (m_currentWaypointIndex, destIndex);

            if (m_currentWaypointIndex == destIndex)
               m_jumpReady = true;
         }
         else
            ResetDoubleJumpState ();
      }
      break;

   // escape from bomb behaviour
   case TASK_ESCAPEFROMBOMB:
      m_aimFlags |= AIM_NAVPOINT;

      if (!g_bombPlanted)
         TaskComplete ();

      if (IsShieldDrawn ())
         pev->button |= IN_ATTACK2;

      if (m_currentWeapon != WEAPON_KNIFE && GetNearbyEnemiesNearPosition (pev->origin, 9999) == 0)
         SelectWeaponByName ("weapon_knife");

      if (DoWaypointNav ()) // reached destination?
      {
         TaskComplete (); // we're done

         // press duck button if we still have some enemies
         if (GetNearbyEnemiesNearPosition (pev->origin, 2048))
            m_campButtons = IN_DUCK;

         // we're reached destination point so just sit down and camp
         PushTask (TASK_CAMP, TASKPRI_CAMP, -1, engine->GetTime () + 10.0f, true);
      }
      else if (!GoalIsValid ()) // didn't choose goal waypoint yet?
      {
         DeleteSearchNodes ();

         int lastSelectedGoal = -1;
         float safeRadius = engine->RandomFloat (1024.0, 2048.0f), minPathDistance = 4096.0f;

         for (i = 0; i < g_numWaypoints; i++)
         {
            if ((g_waypoint->GetPath (i)->origin - g_waypoint->GetBombPosition ()).GetLength () < safeRadius)
               continue;

            float pathDistance = g_waypoint->GetPathDistanceFloat (m_currentWaypointIndex, i);

            if (minPathDistance > pathDistance)
            {
               minPathDistance = pathDistance;
               lastSelectedGoal = i;
            }
         }

         if (lastSelectedGoal < 0)
            lastSelectedGoal = g_waypoint->FindFarest (pev->origin, safeRadius);

         m_prevGoalIndex = lastSelectedGoal;
         GetCurrentTask ()->data = lastSelectedGoal;

         FindShortestPath (m_currentWaypointIndex, lastSelectedGoal);
      }
      break;

   // shooting breakables in the way action
   case TASK_DESTROYBREAKABLE:
      m_aimFlags |= AIM_OVERRIDE;

      // Breakable destroyed?
      if (FNullEnt (FindBreakable ()))
      {
         TaskComplete ();
         break;
      }
      pev->button |= m_campButtons;

      m_checkTerrain = false;
      m_moveToGoal = false;
      m_navTimeset = engine->GetTime ();

      src = m_breakable;
      m_camp = src;

      // is bot facing the breakable?
      if (GetShootingConeDeviation (GetEntity (), &src) >= 0.90)
      {
         m_moveSpeed = 0.0f;
         m_strafeSpeed = 0.0f;

         if (m_currentWeapon == WEAPON_KNIFE)
            SelectBestWeapon ();

         m_wantsToFire = true;
      }
      else
      {
         m_checkTerrain = true;
         m_moveToGoal = true;
      }
      break;

   // picking up items and stuff behaviour
   case TASK_PICKUPITEM:
      if (FNullEnt (m_pickupItem))
      {
         m_pickupItem = null;
         TaskComplete ();

         break;
      }

      destination = GetEntityOrigin (m_pickupItem);
      m_destOrigin = destination;
      m_entity = destination;

      // find the distance to the item
      float itemDistance = (destination - pev->origin).GetLength ();

      switch (m_pickupType)
      {
      case PICKTYPE_WEAPON:
         m_aimFlags |= AIM_NAVPOINT;

         // near to weapon?
         if (itemDistance < 60)
         {
            for (i = 0; i < 7; i++)
            {
               if (strcmp (g_weaponSelect[i].modelName, STRING (m_pickupItem->v.model) + 9) == 0)
                  break;
            }

            if (i < 7)
            {
               // secondary weapon. i.e., pistol
               int weaponID = 0;

               for (i = 0; i < 7; i++)
               {
                  if (pev->weapons & (1 << g_weaponSelect[i].id))
                     weaponID = i;
               }

               if (weaponID > 0)
               {
                  SelectWeaponbyNumber (weaponID);
                  FakeClientCommand (GetEntity (), "drop");

                  if (HasShield ()) // If we have the shield...
                     FakeClientCommand (GetEntity (), "drop"); // discard both shield and pistol
               }
               EquipInBuyzone (0);
            }
            else
            {
               // primary weapon
               int weaponID = GetHighestWeapon ();

               if ((weaponID > 6) || HasShield ())
               {
                  SelectWeaponbyNumber (weaponID);
                  FakeClientCommand (GetEntity (), "drop");
               }
               EquipInBuyzone (0);
            }
            CheckSilencer (); // check the silencer
         }
         break;

      case PICKTYPE_SHIELDGUN:
         m_aimFlags |= AIM_NAVPOINT;

         if (HasShield ())
         {
            m_pickupItem = null;
            break;
         }
         else if (itemDistance < 60) // near to shield?
         {
            // get current best weapon to check if it's a primary in need to be dropped
            int weaponID = GetHighestWeapon ();

            if (weaponID > 6)
            {
               SelectWeaponbyNumber (weaponID);
               FakeClientCommand (GetEntity (), "drop");
            }
         }
         break;

      case PICKTYPE_PLANTEDC4:
         m_aimFlags |= AIM_ENTITY;

         if (team == TEAM_COUNTER && itemDistance < 80)
         {
            ChatterMessage (Chatter_DefusingC4);

            // notify team of defusing
            if (GetNearbyFriendsNearPosition (pev->origin, 600) < 1 && GetNearbyFriendsNearPosition (pev->origin, 9999) >= 1)
               RadioMessage (Radio_NeedBackup);

            m_moveToGoal = false;
            m_checkTerrain = false;

            m_moveSpeed = 0.0f;
            m_strafeSpeed = 0.0f;

            PushTask (TASK_DEFUSEBOMB, TASKPRI_DEFUSEBOMB, -1, 0.0, false);
         }
         break;

      case PICKTYPE_HOSTAGE:
         m_aimFlags |= AIM_ENTITY;
         src = EyePosition ();

         if (!IsAlive (m_pickupItem))
         {
            // don't pickup dead hostages
            m_pickupItem = null;
            TaskComplete ();

            break;
         }

         if (itemDistance < 60)
         {
            float angleToEntity = InFieldOfView (destination - src);

            if (angleToEntity <= 10) // bot faces hostage?
            {
               // use game dll function to make sure the hostage is correctly 'used'
               MDLL_Use (m_pickupItem, GetEntity ());

               if (engine->RandomInt (0, 100) < 80)
                  ChatterMessage (Chatter_UseHostage);

               for (i = 0; i < Const_MaxHostages; i++)
               {
                  if (FNullEnt (m_hostages[i])) // store pointer to hostage so other bots don't steal from this one or bot tries to reuse it
                  {
                     m_hostages[i] = m_pickupItem;
                     m_pickupItem = null;

                     break;
                  }
               }
            }
            m_lastCollTime = engine->GetTime () + 0.1f; // also don't consider being stuck
         }
         break;

      case PICKTYPE_DEFUSEKIT:
         m_aimFlags |= AIM_NAVPOINT;

         if (m_hasDefuser)
         {
            m_pickupItem = null;
            m_pickupType = PICKTYPE_NONE;
         }
         break;

      case PICKTYPE_BUTTON:
         m_aimFlags |= AIM_ENTITY;

         if (FNullEnt (m_pickupItem) || m_buttonPushTime < engine->GetTime ()) // it's safer...
         {
            TaskComplete ();
            m_pickupType = PICKTYPE_NONE;

            break;
         }

         // find angles from bot origin to entity...
         src = EyePosition ();
         float angleToEntity = InFieldOfView (destination - src);

         if (itemDistance < 90) // near to the button?
         {
            m_moveSpeed = 0.0f;
            m_strafeSpeed = 0.0f;
            m_moveToGoal = false;
            m_checkTerrain = false;

            if (angleToEntity <= 10) // facing it directly?
            {
               MDLL_Use (m_pickupItem, GetEntity ());

               m_pickupItem = null;
               m_pickupType = PICKTYPE_NONE;
               m_buttonPushTime = engine->GetTime () + 3.0f;

               TaskComplete ();
            }
         }
         break;
      }
      break;
   }
}

void Bot::BotAI (void)
{
   // this function gets called each frame and is the core of all bot ai. from here all other subroutines are called

   float movedDistance; // length of different vector (distance bot moved)
   TraceResult tr;

   int team = GetTeam (GetEntity ());

   // switch to knife if time to do this
   if (m_checkKnifeSwitch && m_buyingFinished && m_spawnTime + engine->RandomFloat (4.0, 6.5) < engine->GetTime ())
   {
      if (engine->RandomInt (1, 100) < 2 && yb_spraypaints.GetBool ())
         PushTask (TASK_SPRAYLOGO, TASKPRI_SPRAYLOGO, -1, engine->GetTime () + 1.0f, false);

      if (m_skill > 75 && engine->RandomInt (0, 100) < (m_personality == PERSONALITY_RUSHER ? 99 : 80) && !m_isReloading && (g_mapType & (MAP_CS | MAP_DE | MAP_ES | MAP_AS)))
        SelectWeaponByName ("weapon_knife");

      m_checkKnifeSwitch = false;

      if (engine->RandomInt (0, 100) < yb_followpercent.GetInt () && FNullEnt (m_targetEntity) && !m_isLeader && !(pev->weapons & (1 << WEAPON_C4)))
         AttachToUser ();
   }

   // check if we already switched weapon mode
   if (m_checkWeaponSwitch && m_buyingFinished && m_spawnTime + engine->RandomFloat (2.0, 3.5) < engine->GetTime ())
   {
      if (HasShield () && IsShieldDrawn ())
        pev->button |= IN_ATTACK2;
      else
      {
         switch (m_currentWeapon)
         {
         case WEAPON_M4A1:
         case WEAPON_USP:
            CheckSilencer ();
            break;

         case WEAPON_FAMAS:
         case WEAPON_GLOCK18:
            if (engine->RandomInt (0, 100) < 50)
               pev->button |= IN_ATTACK2;
            break;
         }
      }

      // select a leader bot for this team
      SelectLeaderEachTeam (team);
      m_checkWeaponSwitch = false;
   }

   // warning: the following timers aren't frame independent so it varies on slower/faster computers

   // increase reaction time
   m_actualReactionTime += 0.2f;

   if (m_actualReactionTime > m_idealReactionTime)
      m_actualReactionTime = m_idealReactionTime;

   // bot could be blinded by flashbang or smoke, recover from it
   m_viewDistance += 3.0f;

   if (m_viewDistance > m_maxViewDistance)
      m_viewDistance = m_maxViewDistance;

   if (m_blindTime > engine->GetTime ())
      m_maxViewDistance = 4096.0f;

   m_moveSpeed = pev->maxspeed;

   if (m_prevTime <= engine->GetTime ())
   {
      // see how far bot has moved since the previous position...
      movedDistance = (m_prevOrigin - pev->origin).GetLength ();

      // save current position as previous
      m_prevOrigin = pev->origin;
      m_prevTime = engine->GetTime () + 0.2f;
   }
   else
      movedDistance = 2.0f;

   // if there's some radio message to respond, check it
   if (m_radioOrder != 0)
      CheckRadioCommands ();

   // do all sensing, calculate/filter all actions here
   SetConditions ();

   // some stuff required by by chatter engine
   if ((m_states & STATE_SEEINGENEMY) && !FNullEnt (m_enemy))
   {
      if (engine->RandomInt (0, 100) < 45 && GetNearbyFriendsNearPosition (pev->origin, 512) == 0 && (m_enemy->v.weapons & (1 << WEAPON_C4)))
         ChatterMessage (Chatter_SpotTheBomber);

      if (engine->RandomInt (0, 100) < 45 && GetTeam (GetEntity ()) == TEAM_TERRORIST && GetNearbyFriendsNearPosition (pev->origin, 512) == 0 && *g_engfuncs.pfnInfoKeyValue (g_engfuncs.pfnGetInfoKeyBuffer (m_enemy), "model") == 'v')
         ChatterMessage (Chatter_VIPSpotted);

      if (engine->RandomInt (0, 100) < 50 && GetNearbyFriendsNearPosition (pev->origin, 450) == 0 && GetTeam (m_enemy) != GetTeam (GetEntity ()) && IsGroupOfEnemies (m_enemy->v.origin, 2, 384))
         ChatterMessage (Chatter_ScaredEmotion);

      if (engine->RandomInt (0, 100) < 40 && GetNearbyFriendsNearPosition (pev->origin, 1024) == 0 && ((m_enemy->v.weapons & (1 << WEAPON_AWP)) || (m_enemy->v.weapons & (1 << WEAPON_SCOUT)) ||  (m_enemy->v.weapons & (1 << WEAPON_G3SG1)) || (m_enemy->v.weapons & (1 << WEAPON_SG550))))
         ChatterMessage (Chatter_SniperWarning);
   }
   Vector src, dest;

   m_checkTerrain = true;
   m_moveToGoal = true;
   m_wantsToFire = false;

   AvoidGrenades (); // avoid flyings grenades
   m_isUsingGrenade = false;

   RunTask (); // execute current task
   ChooseAimDirection (); // choose aim direction
   FacePosition (); // and turn to chosen aim direction

   // the bots wants to fire at something?
   if (m_wantsToFire && !m_isUsingGrenade && m_shootTime <= engine->GetTime ())
      FireWeapon (); // if bot didn't fire a bullet try again next frame

   // check for reloading
   if (m_reloadCheckTime <= engine->GetTime ())
      CheckReload ();

   // set the reaction time (surprise momentum) different each frame according to skill
   m_idealReactionTime = engine->RandomFloat (g_skillTab[m_skill / 20].minSurpriseTime, g_skillTab[m_skill / 20].maxSurpriseTime);

   // calculate 2 direction vectors, 1 without the up/down component
   Vector directionOld = m_destOrigin - (pev->origin + pev->velocity * m_frameInterval);
   Vector directionNormal = directionOld.Normalize ();

   Vector direction = directionNormal;
   directionNormal.z = 0.0f;

   m_moveAngles = directionOld.ToAngles ();

   m_moveAngles.ClampAngles ();
   m_moveAngles.x *= -1.0f; // invert for engine

#if 0
   if (yb_hardmode.GetBool () && GetCurrentTask ()->taskID == Task_Normal && ((m_aimFlags & AimPos_Enemy) || (m_states & State_SeeingEnemy)) && !IsOnLadder ())
      CombatFight ();
#else
   if (yb_hardmode.GetInt () == 1 && ((m_aimFlags & AIM_ENEMY) || (m_states & (STATE_SEEINGENEMY | STATE_SUSPECTENEMY)) || (GetCurrentTask ()->taskID == TASK_SEEKCOVER && (m_isReloading || m_isVIP))) && !yb_knifemode.GetBool () && GetCurrentTask ()->taskID != TASK_CAMP && !IsOnLadder ())
   {
      m_moveToGoal = false; // don't move to goal
      m_navTimeset = engine->GetTime ();

      if (IsValidPlayer (m_enemy))
         CombatFight ();
   }
#endif
   // check if we need to escape from bomb
   if ((g_mapType & MAP_DE) && g_bombPlanted && m_notKilled && GetCurrentTask ()->taskID != TASK_ESCAPEFROMBOMB && GetCurrentTask ()->taskID != TASK_CAMP && OutOfBombTimer ())
   {
      TaskComplete (); // complete current task

      // then start escape from bomb immidiate
      PushTask (TASK_ESCAPEFROMBOMB, TASKPRI_ESCAPEFROMBOMB, -1, 0.0, true);
   }

   // allowed to move to a destination position?
   if (m_moveToGoal)
   {
      GetValidWaypoint ();

      // Press duck button if we need to
      if ((g_waypoint->GetPath (m_currentWaypointIndex)->flags & WAYPOINT_CROUCH) && !(g_waypoint->GetPath (m_currentWaypointIndex)->flags & WAYPOINT_CAMP))
         pev->button |= IN_DUCK;

      m_timeWaypointMove = engine->GetTime ();

      if (IsInWater ()) // special movement for swimming here
      {
         // check if we need to go forward or back press the correct buttons
         if (InFieldOfView (m_destOrigin - EyePosition ()) > 90)
            pev->button |= IN_BACK;
         else
            pev->button |= IN_FORWARD;

         if (m_moveAngles.x > 60.0f)
            pev->button |= IN_DUCK;
         else if (m_moveAngles.x < -60.0f)
            pev->button |= IN_JUMP;
      }
   }

   if (m_checkTerrain) // are we allowed to check blocking terrain (and react to it)?
   {
      m_isStuck = false;
      edict_t *ent = null;

      // Test if there's a shootable breakable in our way
      if (!FNullEnt (ent = FindBreakable ()))
      {
         m_breakableEntity = ent;
         m_campButtons = pev->button & IN_DUCK;

         PushTask (TASK_DESTROYBREAKABLE, TASKPRI_SHOOTBREAKABLE, -1, 0.0, false);
      }
      else
      {
         ent = null;
         edict_t *pentNearest = null;

         if (FindNearestPlayer (reinterpret_cast <void **> (&pentNearest), GetEntity (), pev->maxspeed, true, false, true, true)) // found somebody?
         {
            MakeVectors (m_moveAngles); // use our movement angles

            // try to predict where we should be next frame
            Vector moved = pev->origin + g_pGlobals->v_forward * m_moveSpeed * m_frameInterval;
            moved = moved + g_pGlobals->v_right * m_strafeSpeed * m_frameInterval;
            moved = moved + pev->velocity * m_frameInterval;

            float nearestDistance = (pentNearest->v.origin - pev->origin).GetLength2D ();
            float nextFrameDistance = ((pentNearest->v.origin + pentNearest->v.velocity * m_frameInterval) - pev->origin).GetLength2D ();

            // is player that near now or in future that we need to steer away?
            if ((pentNearest->v.origin - moved).GetLength2D () <= 48.0f || (nearestDistance <= 56.0f && nextFrameDistance < nearestDistance))
            {
               // to start strafing, we have to first figure out if the target is on the left side or right side
               Vector dirToPoint = (pev->origin - pentNearest->v.origin).SkipZ ();

               if ((dirToPoint | g_pGlobals->v_right.SkipZ ()) > 0.0f)
                  SetStrafeSpeed (directionNormal, pev->maxspeed);
               else
                  SetStrafeSpeed (directionNormal, -pev->maxspeed);

               ResetCollideState ();

               if (nearestDistance < 56.0f && (dirToPoint | g_pGlobals->v_forward.SkipZ ()) < 0.0f)
                  m_moveSpeed = -pev->maxspeed;
            }
         }

         // Standing still, no need to check?
         // FIXME: doesn't care for ladder movement (handled separately) should be included in some way
         if ((m_moveSpeed >= 10 || m_strafeSpeed >= 10) && m_lastCollTime < engine->GetTime ())
         {
            if (movedDistance < 2.0f && m_prevSpeed >= 1.0f) // didn't we move enough previously?
            {
               // Then consider being stuck
               m_prevTime = engine->GetTime ();
               m_isStuck = true;

               if (m_firstCollideTime == 0.0f)
                  m_firstCollideTime = engine->GetTime () + 0.2f;
            }
            else // not stuck yet
            {
               // test if there's something ahead blocking the way
               if (CantMoveForward (directionNormal, &tr) && !IsOnLadder ())
               {
                  if (m_firstCollideTime == 0.0f)
                     m_firstCollideTime = engine->GetTime () + 0.2f;

                  else if (m_firstCollideTime <= engine->GetTime ())
                     m_isStuck = true;
               }
               else
                  m_firstCollideTime = 0.0f;
            }

            if (!m_isStuck) // not stuck?
            {
               if (m_probeTime + 0.5f < engine->GetTime ())
                  ResetCollideState (); // reset collision memory if not being stuck for 0.5 secs
               else
               {
                  // remember to keep pressing duck if it was necessary ago
                  if (m_collideMoves[m_collStateIndex] == COSTATE_DUCK && IsOnFloor () || IsInWater ())
                     pev->button |= IN_DUCK;
               }
            }
            else // bot is stuck!
            {
               // not yet decided what to do?
               if (m_collisionState == COSTATE_UNDECIDED)
               {
                  char bits = 0;

                  if (IsOnLadder ())
                     bits = COPROBE_STRAFE;
                  else if (IsInWater ())
                     bits = (COPROBE_JUMP | COPROBE_STRAFE);
                  else
                     bits = ((engine->RandomInt (0, 10) > 7 ? COPROBE_JUMP : 0) | COPROBE_STRAFE | COPROBE_DUCK);

                  // collision check allowed if not flying through the air
                  if (IsOnFloor () || IsOnLadder () || IsInWater ())
                  {
                     int state[8];
                     int i = 0;

                     // first 4 entries hold the possible collision states
                     state[i++] = COSTATE_JUMP;
                     state[i++] = COSTATE_DUCK;
                     state[i++] = COSTATE_STRAFELEFT;
                     state[i++] = COSTATE_STRAFERIGHT;

                     // now weight all possible states
                     if (bits & COPROBE_JUMP)
                     {
                        state[i] = 0;

                        if (CanJumpUp (directionNormal))
                           state[i] += 10;

                        if (m_destOrigin.z >= pev->origin.z + 18.0f)
                           state[i] += 5;

                        if (EntityIsVisible (m_destOrigin))
                        {
                           MakeVectors (m_moveAngles);

                           src = EyePosition ();
                           src = src + (g_pGlobals->v_right * 15);

                           TraceLine (src, m_destOrigin, true, true, GetEntity (), &tr);

                           if (tr.flFraction >= 1.0f)
                           {
                              src = EyePosition ();
                              src = src - (g_pGlobals->v_right * 15);

                              TraceLine (src, m_destOrigin, true, true, GetEntity (), &tr);

                              if (tr.flFraction >= 1.0f)
                                 state[i] += 5;
                           }
                        }
                        if (pev->flags & FL_DUCKING)
                           src = pev->origin;
                        else
                           src = pev->origin + Vector (0, 0, -17);

                        dest = src + directionNormal * 30;
                        TraceLine (src, dest, true, true, GetEntity (), &tr);

                        if (tr.flFraction != 1.0f)
                           state[i] += 10;
                     }
                     else
                        state[i] = 0;
                     i++;

                     if (bits & COPROBE_DUCK)
                     {
                        state[i] = 0;

                        if (CanDuckUnder (directionNormal))
                           state[i] += 10;

                        if ((m_destOrigin.z + 36.0f <= pev->origin.z) && EntityIsVisible (m_destOrigin))
                           state[i] += 5;
                     }
                     else
                        state[i] = 0;
                     i++;

                     if (bits & COPROBE_STRAFE)
                     {
                        state[i] = 0;
                        state[i + 1] = 0;

                        // to start strafing, we have to first figure out if the target is on the left side or right side
                        MakeVectors (m_moveAngles);

                        Vector dirToPoint = (pev->origin - m_destOrigin).Normalize2D ();
                        Vector rightSide = g_pGlobals->v_right.Normalize2D ();

                        bool dirRight = false;
                        bool dirLeft = false;
                        bool blockedLeft = false;
                        bool blockedRight = false;

                        if (dirToPoint | rightSide > 0)
                           dirRight = true;
                        else
                           dirLeft = true;

                        if (m_moveSpeed > 0)
                           direction = g_pGlobals->v_forward;
                        else
                           direction = -g_pGlobals->v_forward;

                        // now check which side is blocked
                        src = pev->origin + (g_pGlobals->v_right * 32);
                        dest = src + (direction * 32);

                        TraceHull (src, dest, true, head_hull, GetEntity (), &tr);

                        if (tr.flFraction != 1.0f)
                           blockedRight = true;

                        src = pev->origin - (g_pGlobals->v_right * 32);
                        dest = src + (direction * 32);

                        TraceHull (src, dest, true, head_hull, GetEntity (), &tr);

                        if (tr.flFraction != 1.0f)
                           blockedLeft = true;

                        if (dirLeft)
                           state[i] += 5;
                        else
                           state[i] -= 5;

                        if (blockedLeft)
                           state[i] -= 5;

                        i++;

                        if (dirRight)
                           state[i] += 5;
                        else
                           state[i] -= 5;

                        if (blockedRight)
                           state[i] -= 5;
                     }
                     else
                     {
                        state[i] = 0;
                        i++;

                        state[i] = 0;
                     }

                     // weighted all possible moves, now sort them to start with most probable
                     int temp = 0;
                     bool isSorting = false;

                     do
                     {
                        isSorting = false;
                        for (i = 0; i < 3; i++)
                        {
                           if (state[i + 4] < state[i + 5])
                           {
                              temp = state[i];

                              state[i] = state[i + 1];
                              state[i + 1] = temp;

                              temp = state[i + 4];

                              state[i + 4] = state[i + 5];
                              state[i + 5] = temp;

                              isSorting = true;
                           }
                        }
                     } while (isSorting);

                     for (i = 0; i < 4; i++)
                        m_collideMoves[i] = state[i];

                     m_collideTime = engine->GetTime ();
                     m_probeTime = engine->GetTime () + 0.5f;
                     m_collisionProbeBits = bits;
                     m_collisionState = COSTATE_PROBING;
                     m_collStateIndex = 0;
                  }
               }

               if (m_collisionState == COSTATE_PROBING)
               {
                  if (m_probeTime < engine->GetTime ())
                  {
                     m_collStateIndex++;
                     m_probeTime = engine->GetTime () + 0.5f;

                     if (m_collStateIndex > 4)
                     {
                        m_navTimeset = engine->GetTime () - 5.0f;
                        ResetCollideState ();
                     }
                  }

                  if (m_collStateIndex <= 4)
                  {
                     switch (m_collideMoves[m_collStateIndex])
                     {
                     case COSTATE_JUMP:
                        if (IsOnFloor () || IsInWater ())
                           pev->button |= IN_JUMP;
                        break;

                     case COSTATE_DUCK:
                        if (IsOnFloor () || IsInWater ())
                           pev->button |= IN_DUCK;
                        break;

                     case COSTATE_STRAFELEFT:
                        SetStrafeSpeed (directionNormal, -pev->maxspeed);
                        break;

                     case COSTATE_STRAFERIGHT:
                        SetStrafeSpeed (directionNormal, pev->maxspeed);
                        break;
                     }
                  }
               }
            }
         }
      }
   }

   // must avoid a grenade?
   if (m_needAvoidGrenade != 0)
   {
      // Don't duck to get away faster
      pev->button &= ~IN_DUCK;

      m_moveSpeed = -pev->maxspeed;
      m_strafeSpeed = pev->maxspeed * m_needAvoidGrenade;
   }

   // time to reach waypoint
   if (m_navTimeset + GetEstimatedReachTime () < engine->GetTime () && FNullEnt (m_enemy) && m_moveToGoal)
   {
      GetValidWaypoint ();

      // clear these pointers, bot mingh be stuck getting to them
      if (!FNullEnt (m_pickupItem) && m_pickupType != PICKTYPE_PLANTEDC4)
         m_itemIgnore = m_pickupItem;

      m_pickupItem = null;
      m_breakableEntity = null;
      m_itemCheckTime = engine->GetTime () + 5.0f;
      m_pickupType = PICKTYPE_NONE;
   }

   if (m_duckTime > engine->GetTime ())
      pev->button |= IN_DUCK;

   if (pev->button & IN_JUMP)
      m_jumpTime = engine->GetTime ();

   if (m_jumpTime + 0.85 > engine->GetTime ())
   {
      if (!IsOnFloor () && !IsInWater ())
         pev->button |= IN_DUCK;
   }

   if (!(pev->button & (IN_FORWARD | IN_BACK)))
   {
      if (m_moveSpeed > 0)
         pev->button |= IN_FORWARD;
      else if (m_moveSpeed < 0)
         pev->button |= IN_BACK;
   }

   if (!(pev->button & (IN_MOVELEFT | IN_MOVERIGHT)))
   {
      if (m_strafeSpeed > 0)
         pev->button |= IN_MOVERIGHT;
      else if (m_strafeSpeed < 0)
         pev->button |= IN_MOVELEFT;
   }

   static float timeDebugUpdate = 0.0f;

   if (!FNullEnt (g_hostEntity) && yb_debug.GetInt () >= 1)
   {
      int specIndex = g_hostEntity->v.iuser2;

      if (specIndex == ENTINDEX (GetEntity ()))
      {
         static int index, goal, taskID;

         if (m_tasks != null)
         {
            if (taskID != m_tasks->taskID || index != m_currentWaypointIndex || goal != m_tasks->data || timeDebugUpdate < engine->GetTime ())
            {
               taskID = m_tasks->taskID;
               index = m_currentWaypointIndex;
               goal = m_tasks->data;

               char taskName[80];

               switch (taskID)
               {
               case TASK_NORMAL:
                  sprintf (taskName, "Normal");
                  break;

               case TASK_PAUSE:
                  sprintf (taskName, "Pause");
                  break;

               case TASK_MOVETOPOSITION:
                  sprintf (taskName, "MoveToPosition");
                  break;

               case TASK_FOLLOWUSER:
                  sprintf (taskName, "FollowUser");
                  break;

               case TASK_PICKUPITEM:
                  sprintf (taskName, "PickupItem");
                  break;

               case TASK_CAMP:
                  sprintf (taskName, "Camp");
                  break;

               case TASK_PLANTBOMB:
                  sprintf (taskName, "PlantBomb");
                  break;

               case TASK_DEFUSEBOMB:
                  sprintf (taskName, "DefuseBomb");
                  break;

               case TASK_FIGHTENEMY:
                  sprintf (taskName, "AttackEnemy");
                  break;

               case TASK_HUNTENEMY:
                  sprintf (taskName, "HuntEnemy");
                  break;

               case TASK_SEEKCOVER:
                  sprintf (taskName, "SeekCover");
                  break;

               case TASK_THROWHEGRENADE:
                  sprintf (taskName, "ThrowExpGrenade");
                  break;

               case TASK_THROWFBGRENADE:
                  sprintf (taskName, "ThrowFlashGrenade");
                  break;

               case TASK_THROWSMGRENADE:
                  sprintf (taskName, "ThrowSmokeGrenade");
                  break;

               case TASK_DOUBLEJUMP:
                  sprintf (taskName, "PerformDoubleJump");
                  break;

               case TASK_ESCAPEFROMBOMB:
                  sprintf (taskName, "EscapeFromBomb");
                  break;

               case TASK_DESTROYBREAKABLE:
                  sprintf (taskName, "ShootBreakable");
                  break;

               case TASK_HIDE:
                  sprintf (taskName, "Hide");
                  break;

               case TASK_BLINDED:
                  sprintf (taskName, "Blinded");
                  break;

               case TASK_SPRAYLOGO:
                  sprintf (taskName, "SprayLogo");
                  break;
               }

               char enemyName[80], weaponName[80], aimFlags[32], botType[32];

               if (!FNullEnt (m_enemy))
                  strcpy (enemyName, STRING (m_enemy->v.netname));
               else if (!FNullEnt (m_lastEnemy))
               {
                  strcpy (enemyName, " (L)");
                  strcat (enemyName, STRING (m_lastEnemy->v.netname));
               }
               else
                  strcpy (enemyName, " (null)");

               char pickupName[80];

               if (!FNullEnt (m_pickupItem))
                  strcpy (pickupName, STRING (m_pickupItem->v.classname));
               else
                  strcpy (pickupName, " (null)");

               WeaponSelect *selectTab = &g_weaponSelect[0];
               char weaponCount = 0;

               while (m_currentWeapon != selectTab->id && weaponCount < Const_NumWeapons)
               {
                  selectTab++;
                  weaponCount++;
               }

               // set the aim flags
               sprintf (aimFlags, "%s%s%s%s%s%s%s%s",
                  m_aimFlags & AIM_NAVPOINT ? " NavPoint" : "",
                  m_aimFlags & AIM_CAMP ? " CampPoint" : "",
                  m_aimFlags & AIM_PREDICTENEMY ? " PredictEnemy" : "",
                  m_aimFlags & AIM_LASTENEMY ? " LastEnemy" : "",
                  m_aimFlags & AIM_ENTITY ? " Entity" : "",
                  m_aimFlags & AIM_ENEMY ? " Enemy" : "",
                  m_aimFlags & AIM_GRENADE ? " Grenade" : "",
                  m_aimFlags & AIM_OVERRIDE ? " Override" : "");

               // set the bot type
               sprintf (botType, "%s%s%s", m_personality == PERSONALITY_RUSHER ? " Rusher" : "",
                  m_personality == PERSONALITY_CAREFUL ? " Careful" : "",
                  m_personality == PERSONALITY_NORMAL ? " Normal" : "");

               if (weaponCount >= Const_NumWeapons)
               {
                  // prevent printing unknown message from known weapons
                  switch (m_currentWeapon)
                  {
                  case WEAPON_HEGRENADE:
                     sprintf (weaponName, "weapon_hegrenade");
                     break;

                  case WEAPON_FBGRENADE:
                     sprintf (weaponName, "weapon_flashbang");
                     break;

                  case WEAPON_SMGRENADE:
                     sprintf (weaponName, "weapon_smokegrenade");
                     break;

                  case WEAPON_C4:
                     sprintf (weaponName, "weapon_c4");
                     break;

                  default:
                     sprintf (weaponName, "Unknown! (%d)", m_currentWeapon);
                  }
               }
               else
                  sprintf (weaponName, selectTab->weaponName);

               char outputBuffer[512];
               sprintf (outputBuffer, "\n\n\n\n%s (H:%.1f/A:%.1f)- Task: %d=%s Desire:%.02f\nItem: %s Clip: %d Ammo: %d%s Money: %d AimFlags: %s\nSP=%.02f SSP=%.02f I=%d PG=%d G=%d T: %.02f MT: %d\nEnemy=%s Pickup=%s Type=%s\n", STRING (pev->netname), pev->health, pev->armorvalue, taskID, taskName, GetCurrentTask ()->desire, &weaponName[7], GetAmmoInClip (), GetAmmo (), m_isReloading ? " (R)" : "", m_moneyAmount, aimFlags, m_moveSpeed, m_strafeSpeed, index, m_prevGoalIndex, goal, m_navTimeset - engine->GetTime (), pev->movetype, enemyName, pickupName, botType);

               MESSAGE_BEGIN (MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, null, g_hostEntity);
                  WRITE_BYTE (TE_TEXTMESSAGE);
                  WRITE_BYTE (1);
                  WRITE_SHORT (FixedSigned16 (-1, 1 << 13));
                  WRITE_SHORT (FixedSigned16 (0, 1 << 13));
                  WRITE_BYTE (0);
                  WRITE_BYTE (GetTeam (GetEntity ()) == TEAM_COUNTER ? 0 : 255);
                  WRITE_BYTE (100);
                  WRITE_BYTE (GetTeam (GetEntity ()) != TEAM_COUNTER ? 0 : 255);
                  WRITE_BYTE (0);
                  WRITE_BYTE (255);
                  WRITE_BYTE (255);
                  WRITE_BYTE (255);
                  WRITE_BYTE (0);
                  WRITE_SHORT (FixedUnsigned16 (0, 1 << 8));
                  WRITE_SHORT (FixedUnsigned16 (0, 1 << 8));
                  WRITE_SHORT (FixedUnsigned16 (1.0, 1 << 8));
                  WRITE_STRING (const_cast <const char *> (&outputBuffer[0]));
               MESSAGE_END ();

               timeDebugUpdate = engine->GetTime () + 1.0f;
            }

            // green = destination origin
            // blue = ideal angles
            // red = view angles

            engine->DrawLine (g_hostEntity, EyePosition (), m_destOrigin, Color (0, 255, 0, 250), 10, 0, 5, 1, LINE_ARROW);

            MakeVectors (m_idealAngles);
            engine->DrawLine (g_hostEntity, EyePosition (), EyePosition () + g_pGlobals->v_forward * 300.0f, Color (0, 0, 255, 250), 10, 0, 5, 1, LINE_ARROW);

            MakeVectors (pev->v_angle);
            engine->DrawLine (g_hostEntity, EyePosition (), EyePosition () + g_pGlobals->v_forward * 300.0f, Color (255, 0, 0, 250), 10, 0, 5, 1, LINE_ARROW);

            // now draw line from source to destination
            PathNode *node = &m_navNode[0];

            while (node != null)
            {
               src = g_waypoint->GetPath (node->index)->origin;
               node = node->next;

               if (node != null)
                  engine->DrawLine (g_hostEntity, EyePosition (), g_waypoint->GetPath (node->index)->origin, Color (255, 100, 55, 200), 15, 0, 5, 1, LINE_ARROW);
            }
         }
      }
   }

   // save the previous speed (for checking if stuck)
   m_prevSpeed = fabsf (m_moveSpeed);
   m_lastDamageType = -1; // reset damage

   pev->angles.ClampAngles ();
   pev->v_angle.ClampAngles ();
}

bool Bot::HasHostage (void)
{
   for (int i = 0; i < Const_MaxHostages; i++)
   {
      if (!FNullEnt (m_hostages[i]))
      {
         // don't care about dead hostages
         if (m_hostages[i]->v.health <= 0 || (pev->origin - m_hostages[i]->v.origin).GetLength () > 600)
         {
            m_hostages[i] = null;
            continue;
         }
         return true;
      }
   }
   return false;
}

void Bot::ResetCollideState (void)
{
   m_collideTime = 0.0f;
   m_probeTime = 0.0f;

   m_collisionProbeBits = 0;
   m_collisionState = COSTATE_UNDECIDED;
   m_collStateIndex = 0;

   for (int i = 0; i < 4; i++)
      m_collideMoves[i] = 0;
}

int Bot::GetAmmo (void)
{
   if (g_weaponDefs[m_currentWeapon].ammo1 == -1)
      return 0;

   return m_ammo[g_weaponDefs[m_currentWeapon].ammo1];
}

void Bot::TakeDamage (edict_t *inflictor, int damage, int armor, int bits)
{
   // this function gets called from the network message handler, when bot's gets hurt from any
   // other player.

   m_lastDamageType = bits;

   int team = GetTeam (GetEntity ());
   g_exp.CollectGoal (static_cast <int> (pev->health), damage, m_chosenGoalIndex, m_prevGoalIndex, team);

   if (IsValidPlayer (inflictor))
   {
      if (GetTeam (inflictor) == team)
      {
         // attacked by a teammate
         if (yb_tkpunish.GetBool () && pev->health < (m_personality == PERSONALITY_RUSHER ? 98 : 90))
         {
            if (m_seeEnemyTime + 1.7 < engine->GetTime ())
            {
               // alright, die you teamkiller!!!
               m_actualReactionTime = 0.0f;
               m_seeEnemyTime = engine->GetTime ();
               m_enemy = inflictor;
               m_lastEnemy = m_enemy;
               m_lastEnemyOrigin = m_enemy->v.origin;
               m_enemyOrigin = m_enemy->v.origin;

               ChatMessage (CHAT_TEAMATTACK);
            }
         }
         HandleChatterMessage ("#Bot_TeamAttack");
      }
      else
      {
         // attacked by an enemy
         if (pev->health > 70)
         {
            m_agressionLevel += 0.1f;

            if (m_agressionLevel > 1.0f)
               m_agressionLevel = 1.0f;
         }
         else
         {
            m_fearLevel += 0.05f;

            if (m_fearLevel > 1.0f)
               m_fearLevel = 1.0f;
         }
         // stop bot from hiding
         RemoveCertainTask (TASK_HIDE);

         if (FNullEnt (m_enemy))
         {
            m_lastEnemy = inflictor;
            m_lastEnemyOrigin = inflictor->v.origin;

            // FIXME - Bot doesn't necessary sees this enemy
            m_seeEnemyTime = engine->GetTime ();
         }

         if (g_botManager->GetBot (m_enemy))
            g_exp.CollectDamage (GetEntity (), inflictor, static_cast <int> (pev->health), armor + damage, m_goalValue, g_botManager->GetBot (m_enemy)->m_goalValue);
         else
         {
            float side = 0;
            g_exp.CollectDamage (GetEntity (), inflictor, static_cast <int> (pev->health), armor + damage, m_goalValue, side);
         }
      }
   }
   else // hurt by unusual damage like drowning or gas
   {
      // leave the camping/hiding position
      if (!g_waypoint->Reachable (this, g_waypoint->FindNearest (m_destOrigin)))
      {
         DeleteSearchNodes ();
         FindWaypoint ();
      }
   }
}

void Bot::TakeBlinded (Vector fade, int alpha)
{
   // this function gets called by network message handler, when screenfade message get's send
   // it's used to make bot blind froumd the grenade.

   if (fade.x != 255 || fade.y != 255 || fade.z != 255 || alpha <= 200)
      return;

   m_enemy = null;

   m_maxViewDistance = engine->RandomFloat (10, 20);
   m_blindTime = engine->GetTime () + static_cast <float> (alpha - 200) / 16;

   if (m_skill <= 80)
   {
      m_blindMoveSpeed = 0.0f;
      m_blindSidemoveSpeed = 0.0f;
      m_blindButton = IN_DUCK;
   }
   else if (m_skill < 99 || m_skill == 100)
   {
      m_blindMoveSpeed = -pev->maxspeed;
      m_blindSidemoveSpeed = 0.0f;

      float walkSpeed = GetWalkSpeed ();

      if (engine->RandomInt (0, 100) > 50)
         m_blindSidemoveSpeed = walkSpeed;
      else
         m_blindSidemoveSpeed = -walkSpeed;

      if (pev->health < 85)
         m_blindMoveSpeed = -GetWalkSpeed ();
      else if (m_personality == PERSONALITY_CAREFUL)
      {
         m_blindMoveSpeed = 0.0f;
         m_blindButton = IN_DUCK;
      }
      else
         m_blindMoveSpeed = walkSpeed;
   }
}

void Bot::HandleChatterMessage (const char *tempMessage)
{
   // this function is added to prevent engine crashes with: 'Message XX started, before message XX ended', or something.

   if (FStrEq (tempMessage, "#CTs_Win") && (GetTeam (GetEntity ()) == TEAM_COUNTER))
   {
      if (g_timeRoundMid > engine->GetTime ())
         ChatterMessage (Chatter_QuicklyWonTheRound);
      else
         ChatterMessage (Chatter_WonTheRound);
   }

   if (FStrEq (tempMessage, "#Terrorists_Win") && (GetTeam (GetEntity ()) == TEAM_TERRORIST))
   {
      if (g_timeRoundMid > engine->GetTime ())
         ChatterMessage (Chatter_QuicklyWonTheRound);
      else
         ChatterMessage (Chatter_WonTheRound);
   }

   if (FStrEq (tempMessage, "#Bot_TeamAttack"))
      ChatterMessage (Chatter_FriendlyFire);

   if (FStrEq (tempMessage, "#Bot_NiceShotCommander"))
      ChatterMessage (Chatter_NiceshotCommander);

   if (FStrEq (tempMessage, "#Bot_NiceShotPall"))
      ChatterMessage (Chatter_NiceshotPall);
}

void Bot::ChatMessage (int type, bool isTeamSay)
{
   extern ConVar yb_chat;

   if (g_chatFactory[type].IsEmpty () || !yb_chat.GetBool ())
      return;

   const char *pickedPhrase = g_chatFactory[type].GetRandomElement ();

   if (IsNullString (pickedPhrase))
      return;

   PrepareChatMessage (const_cast <char *> (pickedPhrase));
   PushMessageQueue (isTeamSay ? CMENU_TEAMSAY : CMENU_SAY);
}

void Bot::DiscardWeaponForUser (edict_t *user, bool discardC4)
{
   // this function, asks bot to discard his current primary weapon (or c4) to the user that requsted it with /drop*
   // command, very useful, when i'm don't have money to buy anything... )

   if (IsAlive (user) && m_moneyAmount >= 2000 && HasPrimaryWeapon () && (user->v.origin - pev->origin).GetLength () <= 240)
   {
      m_aimFlags |= AIM_ENTITY;
      m_lookAt = user->v.origin;

      if (discardC4)
      {
         SelectWeaponByName ("weapon_c4");
         FakeClientCommand (GetEntity (), "drop");

         SayText (FormatBuffer ("Here! %s, and now go and setup it!", STRING (user->v.netname)));
      }
      else
      {
         SelectBestWeapon ();
         FakeClientCommand (GetEntity (), "drop");

         SayText (FormatBuffer ("Here the weapon! %s, feel free to use it ;)", STRING (user->v.netname)));
      }

      m_pickupItem = null;
      m_pickupType = PICKTYPE_NONE;
      m_itemCheckTime = engine->GetTime () + 5.0f;

      if (m_inBuyZone)
      {
         m_buyingFinished = false;
         m_buyState = 0;

         PushMessageQueue (CMENU_BUY);
         m_nextBuyTime = engine->GetTime ();
      }
   }
   else
      SayText (FormatBuffer ("Sorry %s, i don't want discard my %s to you!", STRING (user->v.netname), discardC4 ? "bomb" : "weapon"));
}

void Bot::ResetDoubleJumpState (void)
{
   TaskComplete ();
   DeleteSearchNodes ();

   m_doubleJumpEntity = null;
   m_duckForJump = 0.0f;
   m_doubleJumpOrigin = nullvec;
   m_travelStartIndex = -1;
   m_jumpReady = false;
}

void Bot::DebugMsg (const char *format, ...)
{
   if (yb_debug.GetInt () < 2)
      return;

   va_list ap;
   char buffer[1024];

   va_start (ap, format);
   vsprintf (buffer, format, ap);
   va_end (ap);

   ServerPrintNoTag ("%s: %s", STRING (pev->netname), buffer);

   if (yb_debug.GetInt () >= 3)
      AddLogEntry (false, LOG_DEFAULT, "%s: %s", STRING (pev->netname), buffer);
}

Vector Bot::CheckToss (const Vector &start, Vector end)
{
   // this function returns the velocity at which an object should looped from start to land near end.
   // returns null vector if toss is not feasible.

   TraceResult tr;
   float gravity = engine->GetGravity () * 0.55f;

   end = end - pev->velocity;
   end.z -= 15.0f;

   if (fabsf (end.z - start.z) > 500.0f)
      return nullvec;

   Vector midPoint = start + (end - start) * 0.5f;
   TraceHull (midPoint, midPoint + Vector (0, 0, 500), true, head_hull, ENT (pev), &tr);

   if (tr.flFraction < 1.0f)
   {
      midPoint = tr.vecEndPos;
      midPoint.z = tr.pHit->v.absmin.z - 1.0f;
   }

   if ((midPoint.z < start.z) || (midPoint.z < end.z))
      return nullvec;

   float timeOne = sqrtf ((midPoint.z - start.z) / (0.5f * gravity));
   float timeTwo = sqrtf ((midPoint.z - end.z) / (0.5f * gravity));

   if (timeOne < 0.1)
      return nullvec;

   Vector nadeVelocity = (end - start) / (timeOne + timeTwo);
   nadeVelocity.z = gravity * timeOne;

   Vector apex = start + nadeVelocity * timeOne;
   apex.z = midPoint.z;

   TraceHull (start, apex, false, head_hull, ENT (pev), &tr);

   if (tr.flFraction < 1.0f || tr.fAllSolid)
      return nullvec;

   TraceHull (end, apex, true, head_hull, ENT (pev), &tr);

   if (tr.flFraction != 1.0f)
   {
      float dot = -(tr.vecPlaneNormal | (apex - end).Normalize ());

      if (dot > 0.7f || tr.flFraction < 0.8f) // 60 degrees
         return nullvec;
   }
   return nadeVelocity * 0.777f;
}

Vector Bot::CheckThrow (const Vector &start, Vector end)
{
   // this function returns the velocity vector at which an object should be thrown from start to hit end.
   // returns null vector if throw is not feasible.

   Vector nadeVelocity = (end - start);
   TraceResult tr;

   float gravity = engine->GetGravity () * 0.55f;
   float time = nadeVelocity.GetLength () / 195.0f;

   if (time < 0.01f)
      return nullvec;
   else if (time > 2.0f)
      time = 1.2f;

   nadeVelocity = nadeVelocity * (1.0f / time);
   nadeVelocity.z += gravity * time * 0.5f;

   Vector apex = start + (end - start) * 0.5f;
   apex.z += 0.5f * gravity * (time * 0.5f) * (time * 0.5f);

   TraceHull (start, apex, false, head_hull, GetEntity (), &tr);

   if (tr.flFraction != 1.0f)
      return nullvec;

   TraceHull (end, apex, true, head_hull, GetEntity (), &tr);

   if (tr.flFraction != 1.0f || tr.fAllSolid)
   {
      float dot = -(tr.vecPlaneNormal | (apex - end).Normalize ());

      if (dot > 0.7f || tr.flFraction < 0.8f)
         return nullvec;
   }
   return nadeVelocity * 0.7793f;
}

Vector Bot::CheckBombAudible (void)
{
   // this function checks if bomb is can be heard by the bot, calculations done by manual testing.

   if (!g_bombPlanted || (GetCurrentTask ()->taskID == TASK_ESCAPEFROMBOMB))
      return nullvec; // reliability check

   Vector bombOrigin = g_waypoint->GetBombPosition ();

   if (m_skill > 95)
      return bombOrigin;
   
   float timeElapsed = ((engine->GetTime () - g_timeBombPlanted) / engine->GetC4TimerTime ()) * 100;
   float desiredRadius = 768.0f;

   // start the manual calculations
   if (timeElapsed > 85.0f)
      desiredRadius = 4096.0f;
   else if (timeElapsed > 68.0f)
      desiredRadius = 2048.0f;
   else if (timeElapsed > 52.0f)
      desiredRadius = 1280.0f;
   else if (timeElapsed > 28.0f)
      desiredRadius = 1024.0f;

   // we hear bomb if length greater than radius
   if (desiredRadius < (pev->origin - bombOrigin).GetLength2D ())
      return bombOrigin;

   return nullvec;
}

void Bot::MoveToVector (Vector to)
{
   if (to == nullvec)
      return;

   FindPath (m_currentWaypointIndex, g_waypoint->FindNearest (to), 2);
}

void Bot::RunPlayerMovement (void)
{
   // the purpose of this function is to compute, according to the specified computation
   // method, the msec value which will be passed as an argument of pfnRunPlayerMove. This
   // function is called every frame for every bot, since the RunPlayerMove is the function
   // that tells the engine to put the bot character model in movement. This msec value
   // tells the engine how long should the movement of the model extend inside the current
   // frame. It is very important for it to be exact, else one can experience bizarre
   // problems, such as bots getting stuck into each others. That's because the model's
   // bounding boxes, which are the boxes the engine uses to compute and detect all the
   // collisions of the model, only exist, and are only valid, while in the duration of the
   // movement. That's why if you get a pfnRunPlayerMove for one bot that lasts a little too
   // short in comparison with the frame's duration, the remaining time until the frame
   // elapses, that bot will behave like a ghost : no movement, but bullets and players can
   // pass through it. Then, when the next frame will begin, the stucking problem will arise !

   int msecMethod = yb_msectype.GetInt ();

   if (msecMethod < 1 || msecMethod > 4)
      msecMethod = m_msecBuiltin;

   // this is a pierre-marie baty's method for calculation the msec value, thanks to jussi kivilinna for additional code
   if (msecMethod == 2)
   {
      m_msecVal = static_cast <uint8_t> (m_frameInterval * 1000.0f);
      m_msecDel += m_frameInterval * 1000.0f - m_msecVal;

      // remove effect of integer conversion
      if(m_msecDel > 1.5)
      {
         m_msecVal += 1;
         m_msecDel -= 1;
      }

      if (m_msecVal < 1) // don't allow msec to be less than 1...
         m_msecVal = 1;

      if (m_msecVal > 250) // ...or greater than 250
         m_msecVal = 250;
   }

   // this is rich whitehouse's method for calculating the msec value.
   else if (msecMethod == 3)
   {
      if (m_msecDel > engine->GetTime ())
         m_msecNum += 1.0f;
      else
      {
         if (m_msecNum > 0.0f)
            m_msecVal = static_cast <uint8_t> (450.0f / m_msecNum);

         m_msecDel = engine->GetTime () + 0.5f; // next check in half a second
         m_msecNum = 0.0f;
      }
   }

   // this is tobias heimann's method for calculating the msec value.
   else if (msecMethod == 1)
   {
      if (((m_msecDel + (m_msecNum / 1000.0f)) < engine->GetTime () - 0.51) || (m_msecDel > engine->GetTime ()))
      {
         m_msecDel = engine->GetTime () - 0.05f;
         m_msecNum = 0.0f;
      }

      m_msecVal = static_cast <uint8_t> ((engine->GetTime () - m_msecDel) * 1000.0f - m_msecNum);
      m_msecNum = (engine->GetTime () - m_msecDel) * 1000.0f;

      if (m_msecNum > 1000.0f)
      {
         m_msecDel += m_msecNum / 1000.0f;
         m_msecNum = 0.0f;
      }
   }

   // this is leon hartwig's method for computing the msec value
   else if (msecMethod == 4)
   {
      m_msecVal = static_cast <uint8_t> ((engine->GetTime () - m_msecInterval) * 1000.0f);

      // update msec interval for last calculation method
      m_msecInterval = engine->GetTime ();
   }

   // validate range of the msec value
   if (m_msecVal > 200)
      m_msecVal = 200;
   else if (m_msecVal < 5)
      m_msecVal = 5;

   (*g_engfuncs.pfnRunPlayerMove) (GetEntity (), m_moveAngles, m_moveSpeed, m_strafeSpeed, 0.0, static_cast <unsigned short> (pev->button), static_cast <uint8_t> (pev->impulse), m_msecVal);
}

void Bot::CheckBurstMode (float distance)
{
   // this function checks burst mode, and switch it depending distance to to enemy.

   if (HasShield ())
      return; // no checking when shiled is active

   // if current weapon is glock, disable burstmode on long distances, enable it else
   if (m_currentWeapon == WEAPON_GLOCK18 && distance < 300.0f && m_weaponBurstMode == BURST_DISABLED)
      pev->button |= IN_ATTACK2;
   else if (m_currentWeapon == WEAPON_GLOCK18 && distance >= 30.0f && m_weaponBurstMode == BURST_ENABLED)
      pev->button |= IN_ATTACK2;

   // if current weapon is famas, disable burstmode on short distances, enable it else
   if (m_currentWeapon == WEAPON_FAMAS && distance > 400.0f && m_weaponBurstMode == BURST_DISABLED)
      pev->button |= IN_ATTACK2;
   else if (m_currentWeapon == WEAPON_FAMAS && distance <= 400.0f && m_weaponBurstMode == BURST_ENABLED)
      pev->button |= IN_ATTACK2;
}

void Bot::CheckSilencer (void)
{
   if ((m_currentWeapon == WEAPON_USP && m_skill < 90) || m_currentWeapon == WEAPON_M4A1 && !HasShield ())
   {
      int random = (m_personality == PERSONALITY_RUSHER ? 35 : 65);

      // aggressive bots don't like the silencer
      if (engine->RandomInt (1, 100) <= (m_currentWeapon == WEAPON_USP ? random / 3 : random))
      {
         if (pev->weaponanim > 6) // is the silencer not attached...
            pev->button |= IN_ATTACK2; // attach the silencer
      }
      else
      {
         if (pev->weaponanim <= 6) // is the silencer attached...
            pev->button |= IN_ATTACK2; // detach the silencer
      }
   }
}

float Bot::GetBombTimeleft (void)
{
   if (!g_bombPlanted)
      return 0.0f;

   float timeLeft = ((g_timeBombPlanted + engine->GetC4TimerTime ()) - engine->GetTime ());

   if (timeLeft < 0.0f)
      return 0.0f;

   return timeLeft;
}

float Bot::GetEstimatedReachTime (void)
{
   float estimatedTime = 5.0f; // time to reach next waypoint

   // calculate 'real' time that we need to get from one waypoint to another
   if (m_currentWaypointIndex >= 0 && m_currentWaypointIndex < g_numWaypoints && m_prevWptIndex[0] >= 0 && m_prevWptIndex[0] < g_numWaypoints)
   {
      float distance = (g_waypoint->GetPath (m_prevWptIndex[0])->origin - g_waypoint->GetPath (m_currentWaypointIndex)->origin).GetLength ();

      // caclulate estimated time
      if (pev->maxspeed <= 0.0f)
         estimatedTime = 5.0f * distance / 240.0f;
      else
         estimatedTime = 5.0f * distance / pev->maxspeed;

      // check for special waypoints, that can slowdown our movement
      if ((g_waypoint->GetPath (m_currentWaypointIndex)->flags & WAYPOINT_CROUCH) || (g_waypoint->GetPath (m_currentWaypointIndex)->flags & WAYPOINT_LADDER) || (pev->button & IN_DUCK))
         estimatedTime *= 3.0f;

      // check for too low values
      if (estimatedTime < 3.0f)
         estimatedTime = 3.0f;

      // check for too high values
      if (estimatedTime > 8.0f)
         estimatedTime = 8.0f;
   }
   return estimatedTime;
}

bool Bot::OutOfBombTimer (void)
{
   if (m_currentWaypointIndex == -1 || ((g_mapType & MAP_DE) && (m_hasProgressBar || GetCurrentTask ()->taskID == TASK_ESCAPEFROMBOMB)))
      return false; // if CT bot already start defusing, or already escaping, return false

   // calculate left time
   float timeLeft = GetBombTimeleft ();

   // if time left greater than 13, no need to do other checks
   if (timeLeft > 13)
      return false;

   Vector bombOrigin = g_waypoint->GetBombPosition ();

   // for terrorist, if timer is lower than eleven seconds, return true
   if (static_cast <int> (timeLeft) < 12 && GetTeam (GetEntity ()) == TEAM_TERRORIST && (bombOrigin - pev->origin).GetLength () < 1000)
      return true;

   bool hasTeammatesWithDefuserKit = false;

   // check if our teammates has defusal kit
   for (int i = 0; i < engine->GetMaxClients (); i++)
   {
      Bot *bot = null; // temporaly pointer to bot

      // search players with defuse kit
      if ((bot = g_botManager->GetBot (i)) != null && GetTeam (bot->GetEntity ()) == TEAM_COUNTER && bot->m_hasDefuser && (bombOrigin - bot->pev->origin).GetLength () < 500)
      {
         hasTeammatesWithDefuserKit = true;
         break;
      }
   }

   // add reach time to left time
   float reachTime = g_waypoint->GetTravelTime (pev->maxspeed, g_waypoint->GetPath (m_currentWaypointIndex)->origin, bombOrigin);

   // for counter-terrorist check alos is we have time to reach position plus average defuse time
   if ((timeLeft < reachTime + 10.0f && !m_hasDefuser && !hasTeammatesWithDefuserKit) || (timeLeft < reachTime + 5.0f && m_hasDefuser))
      return true;

   if (m_hasProgressBar && IsOnFloor () && (m_hasDefuser ? 6.0f : 11.0f > GetBombTimeleft ()))
      return true;

   return false; // return false otherwise
}

void Bot::ReactOnSound (void)
{
   int ownIndex = GetIndex ();
   float ownSoundLast = 0.0f;

   if (g_clients[ownIndex].timeSoundLasting > engine->GetTime ())
   {
      if (g_clients[ownIndex].maxTimeSoundLasting <= 0.0f)
         g_clients[ownIndex].maxTimeSoundLasting = 0.5f;

      ownSoundLast = (g_clients[ownIndex].hearingDistance * 0.2f) * (g_clients[ownIndex].timeSoundLasting - engine->GetTime ()) / g_clients[ownIndex].maxTimeSoundLasting;
   }
   edict_t *player = null;

   float maxVolume = 0.0, volume = 0.0, hearEnemyDistance = 0.0f;
   int hearEnemyIndex = -1;

   // loop through all enemy clients to check for hearable stuff
   for (int i = 0; i < engine->GetMaxClients (); i++)
   {
      if (!(g_clients[i].flags & CFLAG_USED) || !(g_clients[i].flags & CFLAG_ALIVE) || g_clients[i].ent == GetEntity () || g_clients[i].timeSoundLasting < engine->GetTime ())
         continue;

      float distance = (g_clients[i].soundPosition - pev->origin).GetLength ();
      float hearingDistance = g_clients[i].hearingDistance;

      if (distance > hearingDistance)
         continue;

      if (g_clients[i].maxTimeSoundLasting <= 0.0f)
         g_clients[i].maxTimeSoundLasting = 0.5f;

      if (distance <= 0.5f * hearingDistance)
         volume = hearingDistance * (g_clients[i].timeSoundLasting - engine->GetTime ()) / g_clients[i].maxTimeSoundLasting;
      else
         volume = 2.0f * hearingDistance * (1.0f - distance / hearingDistance) * (g_clients[i].timeSoundLasting - engine->GetTime ()) / g_clients[i].maxTimeSoundLasting;

      if (g_clients[hearEnemyIndex].team == GetTeam (GetEntity ()) && yb_csdmplay.GetInt () != 2)
         volume = 0.3f * volume;

      // we will care about the most hearable sound instead of the closest one - KWo
      if (volume < maxVolume)
         continue;

      maxVolume = volume;

      if (volume < ownSoundLast)
         continue;

      hearEnemyIndex = i;
      hearEnemyDistance = distance;
   }

   if (hearEnemyIndex >= 0)
   {
      if (g_clients[hearEnemyIndex].team != GetTeam (GetEntity ()) && yb_csdmplay.GetInt () != 2)
         player = g_clients[hearEnemyIndex].ent;
   }

   // did the bot hear someone ?
   if (!FNullEnt (player) && hearEnemyDistance < 2048.0f)
   {
      // change to best weapon if heard something
      if (!(m_states & STATE_SEEINGENEMY) && IsOnFloor () && m_currentWeapon != WEAPON_C4 && m_currentWeapon != WEAPON_HEGRENADE && m_currentWeapon != WEAPON_SMGRENADE && m_currentWeapon != WEAPON_FBGRENADE && !yb_knifemode.GetBool ())
         SelectBestWeapon ();

      m_heardSoundTime = engine->GetTime () + 5.0f;
      m_states |= STATE_HEARENEMY;

      if ((engine->RandomInt (0, 100) < 25) && FNullEnt (m_enemy) && FNullEnt (m_lastEnemy) && m_seeEnemyTime + 7.0f < engine->GetTime ())
         ChatterMessage (Chatter_HeardEnemy);

      m_aimFlags |= AIM_LASTENEMY; 

      // didn't bot already have an enemy ? take this one...
      if (m_lastEnemyOrigin == nullvec || m_lastEnemy == null)
      {
         m_lastEnemy = player;
         m_lastEnemyOrigin = player->v.origin;
      }
      else // bot had an enemy, check if it's the heard one
      {
         if (player == m_lastEnemy)
         {
            // bot sees enemy ? then bail out !
            if (m_states & STATE_SEEINGENEMY)
               return;

            m_lastEnemyOrigin = player->v.origin;
         }
         else
         {
            // if bot had an enemy but the heard one is nearer, take it instead
            float distance = (m_lastEnemyOrigin - pev->origin).GetLength ();

            if (distance > (player->v.origin - pev->origin).GetLength () && m_seeEnemyTime + 2.0f < engine->GetTime ())
            {
               m_lastEnemy = player;
               m_lastEnemyOrigin = player->v.origin;
            }
            else
               return;
         }
      }
      extern ConVar yb_thruwalls;

      // check if heard enemy can be seen
      if (CheckVisibility (VARS (player), &m_lastEnemyOrigin, &m_visibility))
      {
         m_enemy = player;
         m_lastEnemy = player;
         m_enemyOrigin = m_lastEnemyOrigin;

         m_states |= STATE_SEEINGENEMY;
         m_seeEnemyTime = engine->GetTime ();
      }
      else if (m_lastEnemy == player && m_seeEnemyTime + 4.0f > engine->GetTime () && yb_thruwalls.GetBool () && (engine->RandomInt (1, 100) < g_skillTab[m_skill / 20].heardShootThruProb) && IsShootableThruObstacle (player->v.origin))
      {
         m_enemy = player;
         m_lastEnemy = player;
         m_lastEnemyOrigin = player->v.origin;

         m_states |= STATE_SEEINGENEMY;
         m_seeEnemyTime = engine->GetTime ();
      }
   }
}

bool Bot::IsShootableBreakable (edict_t *ent)
{
   // this function is checking that pointed by ent pointer obstacle, can be destroyed.

   if (FClassnameIs (ent, "func_breakable") || (FClassnameIs (ent, "func_pushable") && (ent->v.spawnflags & SF_PUSH_BREAKABLE)))
      return (ent->v.takedamage != DAMAGE_NO) && ent->v.impulse <= 0 && !(ent->v.flags & FL_WORLDBRUSH) && !(ent->v.spawnflags & SF_BREAK_TRIGGER_ONLY) && ent->v.health < 500;

   return false;
}

void Bot::EquipInBuyzone (int iBuyCount)
{
   // this function is gets called when bot enters a buyzone, to allow bot to buy some stuff

   static float lastEquipTime = 0.0f;

   // if bot is in buy zone, try to buy ammo for this weapon...
   if (lastEquipTime + 15.0f < engine->GetTime () && m_inBuyZone && g_timeRoundStart + engine->RandomFloat (10.0, 20.0f) + engine->GetBuyTime () < engine->GetTime () && !g_bombPlanted && m_moneyAmount > g_botBuyEconomyTable[0])
   {
      m_buyingFinished = false;
      m_buyState = iBuyCount;

      // push buy message
      PushMessageQueue (CMENU_BUY);

      m_nextBuyTime = engine->GetTime ();
      lastEquipTime = engine->GetTime ();
   }
}

bool Bot::IsBombDefusing (Vector bombOrigin)
{
   // this function finds if somebody currently defusing the bomb.

   bool defusingInProgress = false;

   for (int i = 0; i < engine->GetMaxClients (); i++)
   {
      Bot *bot = g_botManager->GetBot (i);

      if (bot == null || bot == this)
         continue; // skip invalid bots

      if (GetTeam (GetEntity ()) != GetTeam (bot->GetEntity ()) || bot->GetCurrentTask ()->taskID == TASK_ESCAPEFROMBOMB)
         continue; // skip other mess

      if ((bot->pev->origin - bombOrigin).GetLength () < 80 && (bot->GetCurrentTask ()->taskID == TASK_DEFUSEBOMB || bot->m_hasProgressBar))
      {
         defusingInProgress = true;
         break;
      }

      // take in account peoples too
      if (defusingInProgress || !(g_clients[i].flags & CFLAG_USED) || !(g_clients[i].flags & CFLAG_ALIVE) || g_clients[i].team != GetTeam (GetEntity ()) || IsValidBot (g_clients[i].ent))
         continue;

      if ((g_clients[i].ent->v.origin - bombOrigin).GetLength () < 80)
      {
         defusingInProgress = true;
         break;
      }
   }
   return defusingInProgress;
}
