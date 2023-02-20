/*
Dynamic Script Engine Plugin for Touch Portal
Copyright Maxim Paperno; all rights reserved.

This file may be used under the terms of the GNU
General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

A copy of the GNU General Public License is available at <http://www.gnu.org/licenses/>.

This project may also use 3rd-party Open Source software under the terms
of their respective licenses. The copyright notice above does not apply
to any 3rd-party components used within.
*/

#pragma once

#include <QMetaEnum>
#include <QObject>

namespace DseNS
{
Q_NAMESPACE

//! Type of script engine instances.  \sa DynamicScript.engineType
enum EngineInstanceType : quint8 {
	UnknownInstanceType,  //!< Unknown engine instance type.
	SharedInstance,       //!< Shared engine instance type.
	PrivateInstance,      //!< Private engine instance type.
};
Q_ENUM_NS(EngineInstanceType)

//! Input types for script actions.  \sa DynamicScript.inputType
enum ScriptInputType : quint8 {
	UnknownInputType,  //!< Unknown script input type.
	ExpressionInput,   //!< Expression input type.
	ScriptInput,       //!< Script file input type.
	ModuleInput,       //!< Module file script input type.
};
Q_ENUM_NS(ScriptInputType)

//! Script instance persistence types. \sa DynamicScript.persistence
enum PersistenceType : quint8 {
	PersistSession,     //!< Instance exists for the duration of current runtime sessions only.
	PersistTemporary,   //!< Instance is deleted soon after evaluation.
	PersistSave,        //!< Instance is saved to persistent settings at exit and restored at startup.
};
Q_ENUM_NS(PersistenceType)

//! Script instance saved default value type. These values determine what happens when a script instance is restored from persistent storage.
//! \sa DynamicScript.defaultType
enum SavedDefaultType : quint8 {
	NoSavedDefault,      //!< The instance is not saved in persistent settings, default value type is not applicatble.
	FixedValueDefault,   //!< Instance is created with a fixed default or empty value (specified in `DynamicScript.defaultValue`)
	CustomExprDefault,   //!< Instance is created with default value coming from evaluating a custom expression (specified in `DynamicScript.defaultValue`)
	LastExprDefault,     //!< Instance is created with default value coming from evaluating the last saved primary expression
	                     //!  (exactly as last sent from the corresponding Touch Portal action/connector).
};
Q_ENUM_NS(SavedDefaultType)

//! Defines how an action should behave when it is being "activated" as in the case of a button.
//! A button technically how two transition states -- when pressed and when released. The time between those events
//! can also either be ignored, reacting only to the transition(s), or used to do something,
//! like repeat some command over and over. An action may also invoke different functions on initial press vs. on release.
//! \sa DynamicScript.activation
enum ActivationBehavior : quint8 {
	NoActivation  = 0x00,   //!< The action is never activated, evaluation never happens.
	OnPress       = 0x01,   //!< Evaluation happens on initial activation (eg: button press).
	OnRelease     = 0x02,   //!< Evaluation happens on de-activation (eg: button release).
	RepeatOnHold  = 0x04,   //!< Evaluation happens repeatedly while the action is active (eg: button is held down).
};
Q_FLAG_NS(ActivationBehavior)
//! The `DynamicScript.ActivationBehaviors` type stores an OR combination of `DynamicScript.ActivationBehavior` values.
Q_DECLARE_FLAGS(ActivationBehaviors, ActivationBehavior)

//! Action repeat property type. \sa DSE.setActionRepeat(), DSE.adjustActionRepeat()
enum RepeatProperty : quint8 {
	RepeatRateProperty = 0x01,      //!< Rate, or pause interval between repetitions, in milliseconds.
	RepeatDelayProperty = 0x02,     //!< Initial delay before the the first repetition is activated, in milliseconds.
	AllRepeatProperties = RepeatRateProperty | RepeatDelayProperty   //!< OR combination of Rate and Delay properties.
};
Q_ENUM_NS(RepeatProperty)

// Not public API for now.
// ! How to "adjust" or set a value, eg. in an absolute or relative fashion.
enum AdjustmentType : quint8 {
	SetAbsolute,   // !< Set something to a specific given value.
	SetRelative,   // !< Set something relative to another value; eg. add a positive or negative amount to a current value.
	Increment,     // !< Set something relative to another value by increasing it by a given value. More specific than `SetRelative`.
	Decrement      // !< Set something relative to another value by decreasing it by a given value. More specific than `SetRelative`.
};
Q_ENUM_NS(AdjustmentType)

static inline const QMetaEnum inputTypeMeta() { static const QMetaEnum m = QMetaEnum::fromType<DseNS::ScriptInputType>(); return m; }
static inline const QMetaEnum instanceTypeMeta() { static const QMetaEnum m = QMetaEnum::fromType<DseNS::EngineInstanceType>(); return m; }
static inline const QMetaEnum defaultTypeMeta() { static const QMetaEnum m = QMetaEnum::fromType<DseNS::SavedDefaultType>(); return m; }

}  // namespace

Q_DECLARE_METATYPE(DseNS::EngineInstanceType)
Q_DECLARE_METATYPE(DseNS::ScriptInputType)
Q_DECLARE_METATYPE(DseNS::SavedDefaultType)
Q_DECLARE_METATYPE(DseNS::RepeatProperty)
Q_DECLARE_METATYPE(DseNS::AdjustmentType)
Q_DECLARE_METATYPE(DseNS::ActivationBehavior)
Q_DECLARE_METATYPE(DseNS::ActivationBehaviors)
Q_DECLARE_OPERATORS_FOR_FLAGS(DseNS::ActivationBehaviors)
