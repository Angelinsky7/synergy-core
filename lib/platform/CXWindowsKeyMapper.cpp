/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2003 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "CXWindowsKeyMapper.h"
#include "CXWindowsUtil.h"
#include "CLog.h"
#if defined(X_DISPLAY_MISSING)
#	error X11 is required to build synergy
#else
#	include <X11/X.h>
#	include <X11/Xutil.h>
#	define XK_MISCELLANY
#	define XK_XKB_KEYS
#	include <X11/keysymdef.h>
#	if defined(HAVE_X11_XF86KEYSYM_H)
#		include <X11/XF86keysym.h>
#	endif
#	if !defined(XF86XK_Launch0)
#		define XF86XK_Launch0 0x1008FF40
#	endif
#	if !defined(XF86XK_Launch1)
#		define XF86XK_Launch1 0x1008FF41
#	endif
#endif

// map special KeyID keys to KeySyms
#if defined(HAVE_X11_XF86KEYSYM_H)
static const KeySym		g_mapE000[] =
{
	/* 0x00 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x08 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x10 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x18 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x20 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x28 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x30 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x38 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x40 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x48 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x50 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x58 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x60 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x68 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x70 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x78 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x80 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x88 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x90 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x98 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xa0 */ 0, 0, 0, 0,
	/* 0xa4 */ 0, 0,
	/* 0xa6 */ XF86XK_Back, XF86XK_Forward,
	/* 0xa8 */ XF86XK_Refresh, XF86XK_Stop,
	/* 0xaa */ XF86XK_Search, XF86XK_Favorites,
	/* 0xac */ XF86XK_HomePage, XF86XK_AudioMute,
	/* 0xae */ XF86XK_AudioLowerVolume, XF86XK_AudioRaiseVolume,
	/* 0xb0 */ XF86XK_AudioNext, XF86XK_AudioPrev,
	/* 0xb2 */ XF86XK_AudioStop, XF86XK_AudioPlay,
	/* 0xb4 */ XF86XK_Mail, XF86XK_AudioMedia,
	/* 0xb6 */ XF86XK_Launch0, XF86XK_Launch1,
	/* 0xb8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xc0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xc8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xd0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xd8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xe0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xe8 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xf0 */ 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xf8 */ 0, 0, 0, 0, 0, 0, 0, 0
};
#endif

CXWindowsKeyMapper::CXWindowsKeyMapper()
{
	// do nothing
}

CXWindowsKeyMapper::~CXWindowsKeyMapper()
{
	// do nothing
}

void
CXWindowsKeyMapper::update(Display* display, IKeyState* keyState)
{
	// query which keys are pressed
	char keys[32];
	XQueryKeymap(display, keys);

	// save the auto-repeat mask
	XGetKeyboardControl(display, &m_keyControl);

	// query the pointer to get the keyboard state
	Window root = DefaultRootWindow(display), window;
	int xRoot, yRoot, xWindow, yWindow;
	unsigned int state;
	if (!XQueryPointer(display, root, &root, &window,
								&xRoot, &yRoot, &xWindow, &yWindow, &state)) {
		state = 0;
	}

	// update mappings
	updateKeysymMap(display, keyState);
	updateModifiers();

	// transfer to our state
	for (UInt32 i = 0, j = 0; i < 32; j += 8, ++i) {
		if ((keys[i] & 0x01) != 0)
			keyState->setKeyDown(j + 0);
		if ((keys[i] & 0x02) != 0)
			keyState->setKeyDown(j + 1);
		if ((keys[i] & 0x04) != 0)
			keyState->setKeyDown(j + 2);
		if ((keys[i] & 0x08) != 0)
			keyState->setKeyDown(j + 3);
		if ((keys[i] & 0x10) != 0)
			keyState->setKeyDown(j + 4);
		if ((keys[i] & 0x20) != 0)
			keyState->setKeyDown(j + 5);
		if ((keys[i] & 0x40) != 0)
			keyState->setKeyDown(j + 6);
		if ((keys[i] & 0x80) != 0)
			keyState->setKeyDown(j + 7);
	}

	// set toggle modifier states
	if ((state & LockMask) != 0)
		keyState->setToggled(KeyModifierCapsLock);
	if ((state & m_numLockMask) != 0)
		keyState->setToggled(KeyModifierNumLock);
	if ((state & m_scrollLockMask) != 0)
		keyState->setToggled(KeyModifierScrollLock);
}

KeyButton
CXWindowsKeyMapper::mapKey(IKeyState::Keystrokes& keys,
				const IKeyState& keyState, KeyID id,
				KeyModifierMask desiredMask,
				bool isAutoRepeat) const
{
	// the system translates key events into characters depending
	// on the modifier key state at the time of the event.  to
	// generate the right keysym we need to set the modifier key
	// states appropriately.
	//
	// desiredMask is the mask desired by the caller.  however, there
	// may not be a keycode mapping to generate the desired keysym
	// with that mask.  we override the bits in the mask that cannot
	// be accomodated.

	// convert KeyID to a KeySym
	KeySym keysym = keyIDToKeySym(id, desiredMask);
	if (keysym == NoSymbol) {
		// unknown key
		return 0;
	}

	// get the mapping for this keysym
	KeySymIndex keyIndex = m_keysymMap.find(keysym);

	// if the mapping isn't found and keysym is caps lock sensitive
	// then convert the case of the keysym and try again.
	if (keyIndex == m_keysymMap.end()) {
		KeySym lKey, uKey;
		XConvertCase(keysym, &lKey, &uKey);
		if (lKey != uKey) {
			if (lKey == keysym) {
				keyIndex = m_keysymMap.find(uKey);
			}
			else {
				keyIndex = m_keysymMap.find(lKey);
			}
		}
	}

	if (keyIndex != m_keysymMap.end()) {
		// the keysym is mapped to some keycode.  create the keystrokes
		// for this keysym.
		return mapToKeystrokes(keys, keyState, keyIndex, isAutoRepeat);
	}

	// we can't find the keysym mapped to any keycode.  this doesn't
	// necessarily mean we can't generate the keysym, though.  if the
	// keysym can be created by combining keysyms then we may still
	// be okay.
	CXWindowsUtil::KeySyms decomposition;
	if (!CXWindowsUtil::decomposeKeySym(keysym, decomposition)) {
		return 0;
	}
	LOG((CLOG_DEBUG2 "decomposed keysym 0x%08x into %d keysyms", keysym, decomposition.size()));

	// map each decomposed keysym to keystrokes.  we want the mask
	// and the keycode from the last keysym (which should be the
	// only non-dead key).  the dead keys are not sensitive to
	// anything but shift and mode switch.
	KeyButton keycode = 0;
	for (CXWindowsUtil::KeySyms::const_iterator i = decomposition.begin();
								i != decomposition.end(); ++i) {
		// lookup the key
		keysym   = *i;
		keyIndex = m_keysymMap.find(keysym);
		if (keyIndex == m_keysymMap.end()) {
			// missing a required keysym
			return 0;
		}

		// the keysym is mapped to some keycode
		keycode = mapToKeystrokes(keys, keyState, keyIndex, isAutoRepeat);
		if (keycode == 0) {
			return 0;
		}
	}

	return keycode;
}

KeyModifierMask
CXWindowsKeyMapper::mapModifier(unsigned int state) const
{
	KeyModifierMask mask = 0;
	if (state & ShiftMask)
		mask |= KeyModifierShift;
	if (state & LockMask)
		mask |= KeyModifierCapsLock;
	if (state & ControlMask)
		mask |= KeyModifierControl;
	if (state & m_altMask)
		mask |= KeyModifierAlt;
	if (state & m_metaMask)
		mask |= KeyModifierMeta;
	if (state & m_superMask)
		mask |= KeyModifierSuper;
	if (state & m_modeSwitchMask)
		mask |= KeyModifierModeSwitch;
	if (state & m_numLockMask)
		mask |= KeyModifierNumLock;
	if (state & m_scrollLockMask)
		mask |= KeyModifierScrollLock;
	return mask;
}

void
CXWindowsKeyMapper::updateKeysymMap(Display* display, IKeyState* keyState)
{
	// there are up to 4 keysyms per keycode
	static const unsigned int maxKeysyms = 4;

	// get the number of keycodes
	int minKeycode, maxKeycode;
	XDisplayKeycodes(display, &minKeycode, &maxKeycode);
	const int numKeycodes = maxKeycode - minKeycode + 1;

	// get the keyboard mapping for all keys
	int keysymsPerKeycode;
	KeySym* keysyms = XGetKeyboardMapping(display,
								minKeycode, numKeycodes,
								&keysymsPerKeycode);

	// we only understand up to maxKeysyms keysyms per keycodes
	unsigned int numKeysyms = keysymsPerKeycode;
	if (numKeysyms > maxKeysyms) {
		numKeysyms = maxKeysyms;
	}

	// determine shift and mode switch sensitivity.  a keysym is shift
	// or mode switch sensitive if its keycode is.  a keycode is mode
	// mode switch sensitive if it has keysyms for indices 2 or 3.
	// it's shift sensitive if the keysym for index 1 (if any) is
	// different from the keysym for index 0 and, if the keysym for
	// for index 3 (if any) is different from the keysym for index 2.
	// that is, if shift changes the generated keysym for the keycode.
	std::vector<bool> usesShift(numKeycodes);
	std::vector<bool> usesModeSwitch(numKeycodes);
	for (int i = 0; i < numKeycodes; ++i) {
		// check mode switch first
		if (numKeysyms > 2 &&
			keysyms[i * keysymsPerKeycode + 2] != NoSymbol ||
			keysyms[i * keysymsPerKeycode + 3] != NoSymbol) {
			usesModeSwitch[i] = true;
		}

		// check index 0 with index 1 keysyms
		if (keysyms[i * keysymsPerKeycode + 0] != NoSymbol &&
			keysyms[i * keysymsPerKeycode + 1] != NoSymbol &&
			keysyms[i * keysymsPerKeycode + 1] !=
				keysyms[i * keysymsPerKeycode + 0]) {
			usesShift[i] = true;
		}

		else if (numKeysyms >= 4 &&
			keysyms[i * keysymsPerKeycode + 2] != NoSymbol &&
			keysyms[i * keysymsPerKeycode + 3] != NoSymbol &&
			keysyms[i * keysymsPerKeycode + 3] !=
				keysyms[i * keysymsPerKeycode + 2]) {
			usesShift[i] = true;
		}
	}

	// get modifier map from server
	XModifierKeymap* modifiers   = XGetModifierMapping(display);
	unsigned int keysPerModifier = modifiers->max_keypermod;

	// clear state
	m_keysymMap.clear();
	m_modeSwitchKeysym = NoSymbol;
	m_altMask          = 0;
	m_metaMask         = 0;
	m_superMask        = 0;
	m_modeSwitchMask   = 0;
	m_numLockMask      = 0;
	m_scrollLockMask   = 0;

	// work around for my system, which reports this state bit when
	// mode switch is down, instead of the appropriate modifier bit.
	// should have no effect on other systems.  -crs 9/02.
	m_modeSwitchMask |= (1 << 13);

	// for each modifier keycode, get the index 0 keycode and add it to
	// the keysym map.  also collect all keycodes for each modifier.
	for (unsigned int i = 0; i < 8; ++i) {
		// no keycodes for this modifier yet
		KeyModifierMask mask = 0;
		IKeyState::KeyButtons modifierKeys;

		// add each keycode for modifier
		for (unsigned int j = 0; j < keysPerModifier; ++j) {
			// get keycode and ignore unset keycodes
			KeyCode keycode = modifiers->modifiermap[i * keysPerModifier + j];
			if (keycode == 0) {
				continue;
			}

			// get keysym and get/create key mapping
			const int keycodeIndex = keycode - minKeycode;
			const KeySym keysym    = keysyms[keycodeIndex *
											keysymsPerKeycode + 0];

			// get modifier mask if we haven't yet.  this has the side
			// effect of setting the m_*Mask members.
			if (mask == 0) {
				mask = mapToModifierMask(i, keysym);
				if (mask == 0) {
					continue;
				}
			}

			// save keycode for modifier
			modifierKeys.push_back(keycode);

			// skip if we already have a keycode for this index
			KeyMapping& mapping = m_keysymMap[keysym];
			if (mapping.m_keycode[0] != 0) {
				continue;
			}

			// fill in keysym info
			mapping.m_keycode[0]             = keycode;
			mapping.m_shiftSensitive[0]      = usesShift[keycodeIndex];
			mapping.m_modeSwitchSensitive[0] = usesModeSwitch[keycodeIndex];
			mapping.m_modifierMask           = mask;
			mapping.m_capsLockSensitive      = false;
			mapping.m_numLockSensitive       = false;
		}

		// tell keyState about this modifier
		if (mask != 0 && keyState != NULL) {
			keyState->addModifier(mask, modifierKeys);
		}
	}

	// create a convenient NoSymbol entry (if it doesn't exist yet).
	// sometimes it's useful to handle NoSymbol like a normal keysym.
	// remove any entry for NoSymbol.  that keysym doesn't count.
	{
		KeyMapping& mapping = m_keysymMap[NoSymbol];
		for (unsigned int i = 0; i < numKeysyms; ++i) {
			mapping.m_keycode[i]             = 0;
			mapping.m_shiftSensitive[i]      = false;
			mapping.m_modeSwitchSensitive[i] = false;
		}
		mapping.m_modifierMask      = 0;
		mapping.m_capsLockSensitive = false;
		mapping.m_numLockSensitive  = false;
	}

	// add each keysym to the map, unless we've already inserted a key
	// for that keysym index.
	for (int i = 0; i < numKeycodes; ++i) {
		for (unsigned int j = 0; j < numKeysyms; ++j) {
			// lookup keysym
			const KeySym keysym = keysyms[i * keysymsPerKeycode + j];
			if (keysym == NoSymbol) {
				continue;
			}
			KeyMapping& mapping = m_keysymMap[keysym];

			// skip if we already have a keycode for this index
			if (mapping.m_keycode[j] != 0) {
				continue;
			}

			// fill in keysym info
			if (mapping.m_keycode[0] == 0) {
				mapping.m_modifierMask       = 0;
			}
			mapping.m_keycode[j]             = static_cast<KeyCode>(
												minKeycode + i);
			mapping.m_shiftSensitive[j]      = usesShift[i];
			mapping.m_modeSwitchSensitive[j] = usesModeSwitch[i];
			mapping.m_numLockSensitive       = isNumLockSensitive(keysym);
			mapping.m_capsLockSensitive      = isCapsLockSensitive(keysym);
		}
	}

	// clean up
	XFreeModifiermap(modifiers);
	XFree(keysyms);
}

KeyModifierMask
CXWindowsKeyMapper::mapToModifierMask(unsigned int i, KeySym keysym)
{
	// some modifier indices (0,1,2) are dedicated to particular uses,
	// the rest depend on the keysyms bound.
	switch (i) {
	case 0:
		return KeyModifierShift;

	case 1:
		return KeyModifierCapsLock;

	case 2:
		return KeyModifierControl;

	default:
		switch (keysym) {
		case XK_Shift_L:
		case XK_Shift_R:
			return KeyModifierShift;

		case XK_Control_L:
		case XK_Control_R:
			return KeyModifierControl;

		case XK_Alt_L:
		case XK_Alt_R:
			m_altMask = (1 << i);
			return KeyModifierAlt;

		case XK_Meta_L:
		case XK_Meta_R:
			m_metaMask = (1 << i);
			return KeyModifierMeta;

		case XK_Super_L:
		case XK_Super_R:
			m_superMask = (1 << i);
			return KeyModifierSuper;

		case XK_Mode_switch:
			m_modeSwitchMask = (1 << i);
			return KeyModifierModeSwitch;

		case XK_Caps_Lock:
			return KeyModifierCapsLock;

		case XK_Num_Lock:
			m_numLockMask = (1 << i);
			return KeyModifierNumLock;

		case XK_Scroll_Lock:
			m_scrollLockMask = (1 << i);
			return KeyModifierScrollLock;

		default:
			return 0;
		}
	}
}

void
CXWindowsKeyMapper::updateModifiers()
{
	struct CModifierBitInfo {
	public:
		KeySym CXWindowsKeyMapper::*m_keysym;
		KeySym m_left;
		KeySym m_right;
	};
	static const CModifierBitInfo s_modifierBitTable[] = {
	{ &CXWindowsKeyMapper::m_modeSwitchKeysym, XK_Mode_switch, NoSymbol },
	};

	// choose the keysym to use for some modifiers.  if a modifier has
	// both left and right versions then (arbitrarily) prefer the left.
	for (size_t i = 0; i < sizeof(s_modifierBitTable) /
							sizeof(s_modifierBitTable[0]); ++i) {
		const CModifierBitInfo& info = s_modifierBitTable[i];

		// find available keysym
		KeySymIndex keyIndex = m_keysymMap.find(info.m_left);
		if (keyIndex == m_keysymMap.end() && info.m_right != NoSymbol) {
			keyIndex = m_keysymMap.find(info.m_right);
		}

		// save modifier info
		if (keyIndex                        != m_keysymMap.end() &&
			keyIndex->second.m_modifierMask != 0) {
			this->*(info.m_keysym) = keyIndex->first;
		}
	}

	// if there's no mode switch key mapped then remove all keycodes
	// that depend on it and no keycode can be mode switch sensitive.
	if (m_modeSwitchKeysym == NoSymbol) {
		LOG((CLOG_DEBUG2 "no mode switch in keymap"));
		for (KeySymMap::iterator i = m_keysymMap.begin();
								i != m_keysymMap.end(); ) {
			i->second.m_keycode[2]             = 0;
			i->second.m_keycode[3]             = 0;
			i->second.m_modeSwitchSensitive[0] = false;
			i->second.m_modeSwitchSensitive[1] = false;
			i->second.m_modeSwitchSensitive[2] = false;
			i->second.m_modeSwitchSensitive[3] = false;

			// if this keysym no has no keycodes then remove it
			// except for the NoSymbol keysym mapping.
			if (i->second.m_keycode[0] == 0 && i->second.m_keycode[1] == 0) {
				m_keysymMap.erase(i++);
			}
			else {
				++i;
			}
		}
	}
}

KeySym
CXWindowsKeyMapper::keyIDToKeySym(KeyID id, KeyModifierMask mask) const
{
	// convert id to keysym
	KeySym keysym = NoSymbol;
	if ((id & 0xfffff000) == 0xe000) {
		// special character
		switch (id & 0x0000ff00) {
#if defined(HAVE_X11_XF86KEYSYM_H)
		case 0xe000:
			return g_mapE000[id & 0xff];
#endif

		case 0xee00:
			// ISO 9995 Function and Modifier Keys
			if (id == kKeyLeftTab) {
				keysym = XK_ISO_Left_Tab;
			}
			break;

		case 0xef00:
			// MISCELLANY
			keysym = static_cast<KeySym>(id - 0xef00 + 0xff00);
			break;
		}
	}
	else if ((id >= 0x0020 && id <= 0x007e) ||
			(id >= 0x00a0 && id <= 0x00ff)) {
		// Latin-1 maps directly
		return static_cast<KeySym>(id);
	}
	else {
		// lookup keysym in table
		return CXWindowsUtil::mapUCS4ToKeySym(id);
	}

	// fail if unknown key
	if (keysym == NoSymbol) {
		return keysym;
	}

	// if kKeyTab is requested with shift active then try XK_ISO_Left_Tab
	// instead.  if that doesn't work, we'll fall back to XK_Tab with
	// shift active.  this is to handle primary screens that don't map
	// XK_ISO_Left_Tab sending events to secondary screens that do.
	if (keysym == XK_Tab && (mask & KeyModifierShift) != 0) {
		keysym = XK_ISO_Left_Tab;
	}

	// some keysyms have emergency backups (particularly the numpad
	// keys since most laptops don't have a separate numpad and the
	// numpad overlaying the main keyboard may not have movement
	// key bindings).  figure out the emergency backup.
	KeySym backupKeysym;
	switch (keysym) {
	case XK_KP_Home:
		backupKeysym = XK_Home;
		break;

	case XK_KP_Left:
		backupKeysym = XK_Left;
		break;

	case XK_KP_Up:
		backupKeysym = XK_Up;
		break;

	case XK_KP_Right:
		backupKeysym = XK_Right;
		break;

	case XK_KP_Down:
		backupKeysym = XK_Down;
		break;

	case XK_KP_Prior:
		backupKeysym = XK_Prior;
		break;

	case XK_KP_Next:
		backupKeysym = XK_Next;
		break;

	case XK_KP_End:
		backupKeysym = XK_End;
		break;

	case XK_KP_Insert:
		backupKeysym = XK_Insert;
		break;

	case XK_KP_Delete:
		backupKeysym = XK_Delete;
		break;

	case XK_ISO_Left_Tab:
		backupKeysym = XK_Tab;
		break;

	default:
		backupKeysym = keysym;
		break;
	}

	// see if the keysym is assigned to any keycode.  if not and the
	// backup keysym is then use the backup keysym.
	if (backupKeysym != keysym &&
		m_keysymMap.find(keysym)       == m_keysymMap.end() &&
		m_keysymMap.find(backupKeysym) != m_keysymMap.end()) {
		keysym = backupKeysym;
	}

	return keysym;
}

KeyButton
CXWindowsKeyMapper::mapToKeystrokes(IKeyState::Keystrokes& keys,
				const IKeyState& keyState,
				KeySymIndex keyIndex,
				bool isAutoRepeat) const
{
	// keyIndex must be valid
	assert(keyIndex != m_keysymMap.end());

	KeyModifierMask currentMask = keyState.getActiveModifiers();

	// get the keysym we're trying to generate and possible keycodes
	const KeySym keysym       = keyIndex->first;
	const KeyMapping& mapping = keyIndex->second;
	LOG((CLOG_DEBUG2 "keysym = 0x%08x", keysym));

	// get the best keycode index for the keysym and modifiers.  note
	// that (bestIndex & 1) == 0 if the keycode is a shift modifier
	// and (bestIndex & 2) == 0 if the keycode is a mode switch
	// modifier.  this is important later because we don't want
	// adjustModifiers() to adjust a modifier if that's the key we're
	// mapping.
	unsigned int bestIndex = findBestKeyIndex(keyIndex, currentMask);

	// get the keycode
	KeyButton keycode = mapping.m_keycode[bestIndex];

	// flip low bit of bestIndex if shift is inverted.  if there's a
	// keycode for this new index then use it.  otherwise use the old
	// keycode.  you'd think we should fail if there isn't a keycode
	// for the new index but some keymaps only include the upper case
	// keysyms (notably those on Sun Solaris) so to handle the missing
	// lower case keysyms we just use the old keycode.  note that
	// isShiftInverted() will always return false for a shift modifier.
	if (isShiftInverted(keyIndex, currentMask)) {
		LOG((CLOG_DEBUG2 "shift is inverted"));
		bestIndex ^= 1;
		if (mapping.m_keycode[bestIndex] != 0) {
			keycode = mapping.m_keycode[bestIndex];
		}
	}
	LOG((CLOG_DEBUG2 "bestIndex = %d, keycode = %d", bestIndex, keycode));

	// if this for auto-repeat and this key does not auto-repeat
	// then return 0.
	if (isAutoRepeat &&
		(m_keyControl.auto_repeats[keycode >> 3] &
							static_cast<char>(1 << (keycode & 7))) == 0) {
		return 0;
	}

	// compute desired mask.  the desired mask is the one that matches
	// bestIndex, except if the key being synthesized is a shift key
	// where we desire what we already have or if it's the mode switch
	// key where we only desire to adjust shift.  also, if the keycode
	// is not sensitive to shift then don't adjust it, otherwise
	// something like shift+home would become just home.  similiarly
	// for mode switch.
	KeyModifierMask desiredMask = currentMask;
	if (keyIndex->second.m_modifierMask != KeyModifierShift) {
		if (keyIndex->second.m_shiftSensitive[bestIndex]) {
			if ((bestIndex & 1) != 0) {
				desiredMask |= KeyModifierShift;
			}
			else {
				desiredMask &= ~KeyModifierShift;
			}
		}
		if (keyIndex->second.m_modifierMask != KeyModifierModeSwitch) {
			if (keyIndex->second.m_modeSwitchSensitive[bestIndex]) {
				if ((bestIndex & 2) != 0) {
					desiredMask |= KeyModifierModeSwitch;
				}
				else {
					desiredMask &= ~KeyModifierModeSwitch;
				}
			}
		}
	}

	// adjust the modifiers to match the desired modifiers
	IKeyState::Keystrokes undo;
	if (!adjustModifiers(keys, undo, keyState, desiredMask)) {
		LOG((CLOG_DEBUG2 "failed to adjust modifiers"));
		return 0;
	}

	// add the key event
	IKeyState::Keystroke keystroke;
	keystroke.m_key        = keycode;
	if (!isAutoRepeat) {
		keystroke.m_press  = true;
		keystroke.m_repeat = false;
		keys.push_back(keystroke);
	}
	else {
		keystroke.m_press  = false;
		keystroke.m_repeat = true;
		keys.push_back(keystroke);
		keystroke.m_press  = true;
		keys.push_back(keystroke);
	}

	// put undo keystrokes at end of keystrokes in reverse order
	while (!undo.empty()) {
		keys.push_back(undo.back());
		undo.pop_back();
	}

	return keycode;
}

unsigned int
CXWindowsKeyMapper::findBestKeyIndex(KeySymIndex keyIndex,
				KeyModifierMask /*currentMask*/) const
{
	// there are up to 4 keycodes per keysym to choose from.  the
	// best choice is the one that requires the fewest adjustments
	// to the modifier state.  for example, the letter A normally
	// requires shift + a.  if shift isn't already down we'd have
	// to synthesize a shift press before the a press.  however,
	// if A could also be created with some other keycode without
	// shift then we'd prefer that when shift wasn't down.
	//
	// if the action is an auto-repeat then we don't call this
	// method since we just need to synthesize a key repeat on the
	// same keycode that we pressed.
	// XXX -- do this right
	for (unsigned int i = 0; i < 4; ++i) {
		if (keyIndex->second.m_keycode[i] != 0) {
			return i;
		}
	}

	assert(0 && "no keycode found for keysym");
	return 0;
}

bool
CXWindowsKeyMapper::isShiftInverted(KeySymIndex keyIndex,
				KeyModifierMask currentMask) const
{
	// each keycode has up to 4 keysym associated with it, one each for:
	// no modifiers, shift, mode switch, and shift and mode switch.  if
	// a keysym is modified by num lock and num lock is active then you
	// get the shifted keysym when shift is not down and the unshifted
	// keysym when it is.  that is, num lock inverts the sense of the
	// shift modifier when active.  similarly for caps lock.  this
	// method returns true iff the sense of shift should be inverted
	// for this key given a modifier state.
	if (keyIndex->second.m_numLockSensitive) {
		if ((currentMask & KeyModifierNumLock) != 0) {
			return true;
		}
	}

	// if a keysym is num lock sensitive it is never caps lock
	// sensitive, thus the else here.
	else if (keyIndex->second.m_capsLockSensitive) {
		if ((currentMask & KeyModifierCapsLock) != 0) {
			return true;
		}
	}

	return false;
}

bool
CXWindowsKeyMapper::adjustModifiers(IKeyState::Keystrokes& keys,
				IKeyState::Keystrokes& undo,
				const IKeyState& keyState,
				KeyModifierMask desiredMask) const
{
	KeyModifierMask currentMask = keyState.getActiveModifiers();

	// get mode switch set correctly.  do this before shift because
	// mode switch may be sensitive to the shift modifier and will
	// set/reset it as necessary.
	const bool wantModeSwitch = ((desiredMask & KeyModifierModeSwitch) != 0);
	const bool haveModeSwitch = ((currentMask & KeyModifierModeSwitch) != 0);
	if (wantModeSwitch != haveModeSwitch) {
		LOG((CLOG_DEBUG2 "fix mode switch"));

		// adjust shift if necessary
		KeySymIndex modeSwitchIndex = m_keysymMap.find(m_modeSwitchKeysym);
		assert(modeSwitchIndex != m_keysymMap.end());
		if (modeSwitchIndex->second.m_shiftSensitive[0]) {
			const bool wantShift = false;
			const bool haveShift = ((currentMask & KeyModifierShift) != 0);
			if (wantShift != haveShift) {
				// add shift keystrokes
				LOG((CLOG_DEBUG2 "fix shift for mode switch"));
				if (!keyState.mapModifier(keys, undo,
									KeyModifierShift, wantShift)) {
					return false;
				}
				currentMask ^= KeyModifierShift;
			}
		}

		// add mode switch keystrokes
		if (!keyState.mapModifier(keys, undo,
									KeyModifierModeSwitch, wantModeSwitch)) {
			return false;
		}
		currentMask ^= KeyModifierModeSwitch;
	}

	// get shift set correctly
	const bool wantShift = ((desiredMask & KeyModifierShift) != 0);
	const bool haveShift = ((currentMask & KeyModifierShift) != 0);
	if (wantShift != haveShift) {
		// add shift keystrokes
		LOG((CLOG_DEBUG2 "fix shift"));
		if (!keyState.mapModifier(keys, undo, KeyModifierShift, wantShift)) {
			return false;
		}
		currentMask ^= KeyModifierShift;
	}

	return true;
}

bool
CXWindowsKeyMapper::isNumLockSensitive(KeySym keysym) const
{
	return (IsKeypadKey(keysym) || IsPrivateKeypadKey(keysym));
}

bool
CXWindowsKeyMapper::isCapsLockSensitive(KeySym keysym) const
{
	KeySym lKey, uKey;
	XConvertCase(keysym, &lKey, &uKey);
	return (lKey != uKey);
}


//
// CXWindowsKeyMapper::KeyMapping
//

CXWindowsKeyMapper::KeyMapping::KeyMapping()
{
	m_keycode[0] = 0;
	m_keycode[1] = 0;
	m_keycode[2] = 0;
	m_keycode[3] = 0;
}
