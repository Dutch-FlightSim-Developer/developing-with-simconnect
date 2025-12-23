#pragma once
/*
 * Copyright (c) 2024. Bert Laverman
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#pragma warning(push, 3)

#include <Windows.h>
#include <SimConnect.h>

#pragma warning(pop)


namespace SimConnect {


inline constexpr unsigned long unused{ SIMCONNECT_UNUSED };                             ///< Constant representing an unused value.

/**
 * Constant representing that no identifier has been assigned yet, when using std::optional is not an option.
 */
inline constexpr unsigned long noId{ 0 };

using MessageId = unsigned long;                                                        ///< The type used for message IDs, SIMCONNECT_RECV_ID.
using SendId = unsigned long;                                                           ///< The type used for Send IDs.

namespace Messages {
    using MsgBase = SIMCONNECT_RECV;

    inline constexpr MessageId nullMsg{ SIMCONNECT_RECV_ID_NULL };
    using NullMsg = SIMCONNECT_RECV;

    inline constexpr MessageId exception{ SIMCONNECT_RECV_ID_EXCEPTION };
    using ExceptionMsg = SIMCONNECT_RECV_EXCEPTION;

    inline constexpr MessageId open{ SIMCONNECT_RECV_ID_OPEN };
    using OpenMsg = SIMCONNECT_RECV_OPEN;

    inline constexpr MessageId quit{ SIMCONNECT_RECV_ID_QUIT };
    using QuitMsg = SIMCONNECT_RECV_QUIT;

    inline constexpr MessageId systemState{ SIMCONNECT_RECV_ID_SYSTEM_STATE };
    using SystemStateMsg = SIMCONNECT_RECV_SYSTEM_STATE;

    inline constexpr MessageId flowEvent{ SIMCONNECT_RECV_ID_FLOW_EVENT };
    using FlowEventMsg = SIMCONNECT_RECV_FLOW_EVENT;

    inline constexpr MessageId event{ SIMCONNECT_RECV_ID_EVENT };
    using EventMsg = SIMCONNECT_RECV_EVENT;

    inline constexpr MessageId eventObjectAddRemove{ SIMCONNECT_RECV_ID_EVENT_OBJECT_ADDREMOVE };
    using EventObjectAddRemoveMsg = SIMCONNECT_RECV_EVENT_OBJECT_ADDREMOVE;

    inline constexpr MessageId eventFilename{ SIMCONNECT_RECV_ID_EVENT_FILENAME };
    using EventFilenameMsg = SIMCONNECT_RECV_EVENT_FILENAME;

    inline constexpr MessageId eventFrame{ SIMCONNECT_RECV_ID_EVENT_FRAME };
    using EventFrameMsg = SIMCONNECT_RECV_EVENT_FRAME;

    inline constexpr MessageId eventWeatherMode{ SIMCONNECT_RECV_ID_EVENT_WEATHER_MODE };
    using EventWeatherModeMsg = SIMCONNECT_RECV_EVENT_WEATHER_MODE;

    inline constexpr MessageId eventMultiplayerServerStarted{ SIMCONNECT_RECV_ID_EVENT_MULTIPLAYER_SERVER_STARTED };
    using EventMultiplayerServerStartedMsg = SIMCONNECT_RECV_EVENT_MULTIPLAYER_SERVER_STARTED;

    inline constexpr MessageId eventMultiplayerClientStarted{ SIMCONNECT_RECV_ID_EVENT_MULTIPLAYER_CLIENT_STARTED };
    using EventMultiplayerClientStartedMsg = SIMCONNECT_RECV_EVENT_MULTIPLAYER_CLIENT_STARTED;

    inline constexpr MessageId eventMultiplayerSessionEnded{ SIMCONNECT_RECV_ID_EVENT_MULTIPLAYER_SESSION_ENDED };
    using EventMultiplayerSessionEndedMsg = SIMCONNECT_RECV_EVENT_MULTIPLAYER_SESSION_ENDED;

    inline constexpr MessageId eventRaceEnd{ SIMCONNECT_RECV_ID_EVENT_RACE_END };
    using EventRaceEndMsg = SIMCONNECT_RECV_EVENT_RACE_END;

    inline constexpr MessageId eventRaceLap{ SIMCONNECT_RECV_ID_EVENT_RACE_LAP };
    using EventRaceLapMsg = SIMCONNECT_RECV_EVENT_RACE_LAP;

    inline constexpr MessageId eventEx1{ SIMCONNECT_RECV_ID_EVENT_EX1 };
    using EventEx1Msg = SIMCONNECT_RECV_EVENT_EX1;

    inline constexpr MessageId simObjectData{ SIMCONNECT_RECV_ID_SIMOBJECT_DATA };
    using SimObjectDataMsg = SIMCONNECT_RECV_SIMOBJECT_DATA;

    inline constexpr MessageId simObjectDataByType{ SIMCONNECT_RECV_ID_SIMOBJECT_DATA_BYTYPE };
    using SimObjectDataByTypeMsg = SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE;

    inline constexpr MessageId clientData{ SIMCONNECT_RECV_ID_CLIENT_DATA };
    using ClientDataMsg = SIMCONNECT_RECV_CLIENT_DATA;

    inline constexpr MessageId weatherObservation{ SIMCONNECT_RECV_ID_WEATHER_OBSERVATION };
    using WeatherObservationMsg = SIMCONNECT_RECV_WEATHER_OBSERVATION;

    inline constexpr MessageId cloudState{ SIMCONNECT_RECV_ID_CLOUD_STATE };
    using CloudStateMsg = SIMCONNECT_RECV_CLOUD_STATE;

    inline constexpr MessageId assignedObjectId{ SIMCONNECT_RECV_ID_ASSIGNED_OBJECT_ID };
    using AssignedObjectIdMsg = SIMCONNECT_RECV_ASSIGNED_OBJECT_ID;

    inline constexpr MessageId enumerateSimObjectAndLiveryList{ SIMCONNECT_RECV_ID_ENUMERATE_SIMOBJECT_AND_LIVERY_LIST };
    using EnumerateSimObjectAndLiveryListMsg = SIMCONNECT_RECV_ENUMERATE_SIMOBJECT_AND_LIVERY_LIST;

    inline constexpr MessageId reservedKey{ SIMCONNECT_RECV_ID_RESERVED_KEY };
    using ReservedKeyMsg = SIMCONNECT_RECV_RESERVED_KEY;

    inline constexpr MessageId customAction{ SIMCONNECT_RECV_ID_CUSTOM_ACTION };
    using CustomActionMsg = SIMCONNECT_RECV_CUSTOM_ACTION;

    inline constexpr MessageId airportList{ SIMCONNECT_RECV_ID_AIRPORT_LIST };
    using AirportListMsg = SIMCONNECT_RECV_AIRPORT_LIST;

    inline constexpr MessageId vorList{ SIMCONNECT_RECV_ID_VOR_LIST };
    using VorListMsg = SIMCONNECT_RECV_VOR_LIST;

    inline constexpr MessageId ndbList{ SIMCONNECT_RECV_ID_NDB_LIST };
    using NdbListMsg = SIMCONNECT_RECV_NDB_LIST;

    inline constexpr MessageId waypointList{ SIMCONNECT_RECV_ID_WAYPOINT_LIST };
    using WaypointListMsg = SIMCONNECT_RECV_WAYPOINT_LIST;

    inline constexpr MessageId facilityData{ SIMCONNECT_RECV_ID_FACILITY_DATA };
    using FacilityDataMsg = SIMCONNECT_RECV_FACILITY_DATA;

    inline constexpr MessageId facilityDataEnd{ SIMCONNECT_RECV_ID_FACILITY_DATA_END };
    using FacilityDataEndMsg = SIMCONNECT_RECV_FACILITY_DATA_END;

    inline constexpr MessageId facilityMinimalList{ SIMCONNECT_RECV_ID_FACILITY_MINIMAL_LIST };
    using FacilityMinimalListMsg = SIMCONNECT_RECV_FACILITY_MINIMAL_LIST;

    inline constexpr MessageId jetwayData{ SIMCONNECT_RECV_ID_JETWAY_DATA };
    using JetwayDataMsg = SIMCONNECT_RECV_JETWAY_DATA;

    inline constexpr MessageId controllersList{ SIMCONNECT_RECV_ID_CONTROLLERS_LIST };
    using ControllersListMsg = SIMCONNECT_RECV_CONTROLLERS_LIST;

    inline constexpr MessageId actionCallback{ SIMCONNECT_RECV_ID_ACTION_CALLBACK };
    using ActionCallbackMsg = SIMCONNECT_RECV_ACTION_CALLBACK;

    inline constexpr MessageId enumerateInputEvents{ SIMCONNECT_RECV_ID_ENUMERATE_INPUT_EVENTS };
    using EnumerateInputEventsMsg = SIMCONNECT_RECV_ENUMERATE_INPUT_EVENTS;

    inline constexpr MessageId getInputEvent{ SIMCONNECT_RECV_ID_GET_INPUT_EVENT };
    using GetInputEventMsg = SIMCONNECT_RECV_GET_INPUT_EVENT;

    inline constexpr MessageId subscribeInputEvent{ SIMCONNECT_RECV_ID_SUBSCRIBE_INPUT_EVENT };
    using SubscribeInputEventMsg = SIMCONNECT_RECV_SUBSCRIBE_INPUT_EVENT;

    inline constexpr MessageId enumerateInputEventParams{ SIMCONNECT_RECV_ID_ENUMERATE_INPUT_EVENT_PARAMS };
    using EnumerateInputEventParamsMsg = SIMCONNECT_RECV_ENUMERATE_INPUT_EVENT_PARAMS;
}

using ExceptionCode = unsigned long;                                                    ///< The type used for exception codes.

namespace Exceptions {
    inline constexpr ExceptionCode none{ SIMCONNECT_EXCEPTION_NONE };
    inline constexpr ExceptionCode error{ SIMCONNECT_EXCEPTION_ERROR };
    inline constexpr ExceptionCode sizeMismatch{ SIMCONNECT_EXCEPTION_SIZE_MISMATCH };
    inline constexpr ExceptionCode unrecognizedId{ SIMCONNECT_EXCEPTION_UNRECOGNIZED_ID };
    inline constexpr ExceptionCode unopened{ SIMCONNECT_EXCEPTION_UNOPENED };
    inline constexpr ExceptionCode versionMismatch{ SIMCONNECT_EXCEPTION_VERSION_MISMATCH };
    inline constexpr ExceptionCode tooManyGroups{ SIMCONNECT_EXCEPTION_TOO_MANY_GROUPS };
    inline constexpr ExceptionCode nameUnrecognized{ SIMCONNECT_EXCEPTION_NAME_UNRECOGNIZED };
    inline constexpr ExceptionCode tooManyEventNames{ SIMCONNECT_EXCEPTION_TOO_MANY_EVENT_NAMES };
    inline constexpr ExceptionCode eventIdDuplicate{ SIMCONNECT_EXCEPTION_EVENT_ID_DUPLICATE };
    inline constexpr ExceptionCode tooManyMaps{ SIMCONNECT_EXCEPTION_TOO_MANY_MAPS };
    inline constexpr ExceptionCode tooManyObjects{ SIMCONNECT_EXCEPTION_TOO_MANY_OBJECTS };
    inline constexpr ExceptionCode tooManyRequests{ SIMCONNECT_EXCEPTION_TOO_MANY_REQUESTS };
    inline constexpr ExceptionCode weatherInvalidPort{ SIMCONNECT_EXCEPTION_WEATHER_INVALID_PORT };
    inline constexpr ExceptionCode weatherInvalidMetar{ SIMCONNECT_EXCEPTION_WEATHER_INVALID_METAR };
    inline constexpr ExceptionCode weatherUnableToGetObservation{ SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_GET_OBSERVATION };
    inline constexpr ExceptionCode weatherUnableToCreateStation{ SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_CREATE_STATION };
    inline constexpr ExceptionCode weatherUnableToRemoveStation{ SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_REMOVE_STATION };
    inline constexpr ExceptionCode invalidDataType{ SIMCONNECT_EXCEPTION_INVALID_DATA_TYPE };
    inline constexpr ExceptionCode invalidDataSize{ SIMCONNECT_EXCEPTION_INVALID_DATA_SIZE };
    inline constexpr ExceptionCode dataError{ SIMCONNECT_EXCEPTION_DATA_ERROR };
    inline constexpr ExceptionCode invalidArray{ SIMCONNECT_EXCEPTION_INVALID_ARRAY };
    inline constexpr ExceptionCode createObjectFailed{ SIMCONNECT_EXCEPTION_CREATE_OBJECT_FAILED };
    inline constexpr ExceptionCode loadFlightplanFailed{ SIMCONNECT_EXCEPTION_LOAD_FLIGHTPLAN_FAILED };
    inline constexpr ExceptionCode operationInvalidForObjectType{ SIMCONNECT_EXCEPTION_OPERATION_INVALID_FOR_OBJECT_TYPE };
    inline constexpr ExceptionCode illegalOperation{ SIMCONNECT_EXCEPTION_ILLEGAL_OPERATION };
    inline constexpr ExceptionCode alreadySubscribed{ SIMCONNECT_EXCEPTION_ALREADY_SUBSCRIBED };
    inline constexpr ExceptionCode invalidEnum{ SIMCONNECT_EXCEPTION_INVALID_ENUM };
    inline constexpr ExceptionCode definitionError{ SIMCONNECT_EXCEPTION_DEFINITION_ERROR };
    inline constexpr ExceptionCode duplicateId{ SIMCONNECT_EXCEPTION_DUPLICATE_ID };
    inline constexpr ExceptionCode datumId{ SIMCONNECT_EXCEPTION_DATUM_ID };
    inline constexpr ExceptionCode outOfBounds{ SIMCONNECT_EXCEPTION_OUT_OF_BOUNDS };
    inline constexpr ExceptionCode alreadyCreated{ SIMCONNECT_EXCEPTION_ALREADY_CREATED };
    inline constexpr ExceptionCode objectOutsideRealityBubble{ SIMCONNECT_EXCEPTION_OBJECT_OUTSIDE_REALITY_BUBBLE };
    inline constexpr ExceptionCode objectContainer{ SIMCONNECT_EXCEPTION_OBJECT_CONTAINER };
    inline constexpr ExceptionCode objectAi{ SIMCONNECT_EXCEPTION_OBJECT_AI };
    inline constexpr ExceptionCode objectAtc{ SIMCONNECT_EXCEPTION_OBJECT_ATC };
    inline constexpr ExceptionCode objectSchedule{ SIMCONNECT_EXCEPTION_OBJECT_SCHEDULE };
    inline constexpr ExceptionCode jetwayData{ SIMCONNECT_EXCEPTION_JETWAY_DATA };
    inline constexpr ExceptionCode actionNotFound{ SIMCONNECT_EXCEPTION_ACTION_NOT_FOUND };
    inline constexpr ExceptionCode notAnAction{ SIMCONNECT_EXCEPTION_NOT_AN_ACTION };
    inline constexpr ExceptionCode incorrectActionParams{ SIMCONNECT_EXCEPTION_INCORRECT_ACTION_PARAMS };
    inline constexpr ExceptionCode getInputEventFailed{ SIMCONNECT_EXCEPTION_GET_INPUT_EVENT_FAILED };
    inline constexpr ExceptionCode setInputEventFailed{ SIMCONNECT_EXCEPTION_SET_INPUT_EVENT_FAILED };
    inline constexpr ExceptionCode internal{ SIMCONNECT_EXCEPTION_INTERNAL };
}

using SimObjectId = unsigned long;                                                      ///< The type used for SimObject IDs.

namespace SimObject {
    inline constexpr SimObjectId user{ SIMCONNECT_OBJECT_ID_USER };                             ///< The user SimObject ID.
    inline constexpr SimObjectId userAircraft{ SIMCONNECT_OBJECT_ID_USER_AIRCRAFT };            ///< The user aircraft SimObject ID.
    inline constexpr SimObjectId userAvatar{ SIMCONNECT_OBJECT_ID_USER_AVATAR };                ///< The user avatar SimObject ID.
    inline constexpr SimObjectId userCurrent{ SIMCONNECT_OBJECT_ID_USER_CURRENT };              ///< The current user SimObject ID.
    inline constexpr SimObjectId max{ SIMCONNECT_OBJECT_ID_MAX };                               ///< The maximum SimObject ID.
}

using SimObjectType = SIMCONNECT_SIMOBJECT_TYPE;                                                ///< The type used for SimObject types.

namespace SimObjectTypes {
    inline constexpr SimObjectType user{ SIMCONNECT_SIMOBJECT_TYPE_USER };                      ///< The user SimObject type.
    inline constexpr SimObjectType userAircraft{ SIMCONNECT_SIMOBJECT_TYPE_USER };              ///< The user aircraft SimObject type.
    inline constexpr SimObjectType all{ SIMCONNECT_SIMOBJECT_TYPE_ALL };                        ///< All SimObject types.
    inline constexpr SimObjectType aircraft{ SIMCONNECT_SIMOBJECT_TYPE_AIRCRAFT };              ///< Aircraft SimObject type.
    inline constexpr SimObjectType helicopter{ SIMCONNECT_SIMOBJECT_TYPE_HELICOPTER };          ///< Helicopter SimObject type.
    inline constexpr SimObjectType boat{ SIMCONNECT_SIMOBJECT_TYPE_BOAT };                      ///< Boat SimObject type.
    inline constexpr SimObjectType ground{ SIMCONNECT_SIMOBJECT_TYPE_GROUND };                  ///< Ground vehicle SimObject type.
    inline constexpr SimObjectType hotAirBalloon{ SIMCONNECT_SIMOBJECT_TYPE_HOT_AIR_BALLOON };  ///< Hot air balloon SimObject type.
    inline constexpr SimObjectType animal{ SIMCONNECT_SIMOBJECT_TYPE_ANIMAL };                  ///< Animal SimObject type.
    inline constexpr SimObjectType userAvatar{ SIMCONNECT_SIMOBJECT_TYPE_USER_AVATAR };         ///< User avatar SimObject type.
    inline constexpr SimObjectType userCurrent{ SIMCONNECT_SIMOBJECT_TYPE_USER_CURRENT };       ///< Current user SimObject type.
};

using RequestId = unsigned long;                ///< The type used for request IDs.

using DataPeriod = SIMCONNECT_PERIOD;           ///< The type used for data request periods.

namespace DataPeriods {
    inline constexpr DataPeriod never{ SIMCONNECT_PERIOD_NEVER };                   ///< Data period never.
    inline constexpr DataPeriod once{ SIMCONNECT_PERIOD_ONCE };                     ///< Data period once.
    inline constexpr DataPeriod visualFrame{ SIMCONNECT_PERIOD_VISUAL_FRAME };      ///< Data period visual frame.
    inline constexpr DataPeriod simFrame{ SIMCONNECT_PERIOD_SIM_FRAME };            ///< Data period sim frame.
    inline constexpr DataPeriod second{ SIMCONNECT_PERIOD_SECOND };                 ///< Data period second.
}

using DataDefinitionId = unsigned long;         ///< The type used for data definition IDs.

using DataType = SIMCONNECT_DATATYPE;                 ///< The type used for data types.

namespace DataTypes {
    inline constexpr DataType invalid{ SIMCONNECT_DATATYPE_INVALID };               ///< Invalid data type.

    inline constexpr DataType int8{ SIMCONNECT_DATATYPE_INT8 };                     ///< 8-bit integer data type.
    inline constexpr DataType int32{ SIMCONNECT_DATATYPE_INT32 };                   ///< 32-bit integer data type.
    inline constexpr DataType int64{ SIMCONNECT_DATATYPE_INT64 };                   ///< 64-bit integer data type.

    inline constexpr DataType float32{ SIMCONNECT_DATATYPE_FLOAT32 };               ///< 32-bit floating point data type.
    inline constexpr DataType float64{ SIMCONNECT_DATATYPE_FLOAT64 };               ///< 64-bit floating point data type.

    inline constexpr DataType string8{ SIMCONNECT_DATATYPE_STRING8 };               ///< 8-character string data type.
    inline constexpr DataType string32{ SIMCONNECT_DATATYPE_STRING32 };             ///< 32-character string data type.
    inline constexpr DataType string64{ SIMCONNECT_DATATYPE_STRING64 };             ///< 64-character string data type.
    inline constexpr DataType string128{ SIMCONNECT_DATATYPE_STRING128 };           ///< 128-character string data type.
    inline constexpr DataType string256{ SIMCONNECT_DATATYPE_STRING256 };           ///< 256-character string data type.
    inline constexpr DataType string260{ SIMCONNECT_DATATYPE_STRING260 };           ///< 260-character string data type.
    inline constexpr DataType stringV{ SIMCONNECT_DATATYPE_STRINGV };               ///< Variable-length string data type.

    inline constexpr DataType initPosition{ SIMCONNECT_DATATYPE_INITPOSITION };     ///< Initial position data type.
    using InitPosition = SIMCONNECT_DATA_INITPOSITION;
    inline constexpr DataType markerState{ SIMCONNECT_DATATYPE_MARKERSTATE };       ///< Marker state data type.
    using MarkerState = SIMCONNECT_DATA_MARKERSTATE;
    inline constexpr DataType waypoint{ SIMCONNECT_DATATYPE_WAYPOINT };             ///< Waypoint data type.
    using Waypoint = SIMCONNECT_DATA_WAYPOINT;
    inline constexpr DataType latLonAlt{ SIMCONNECT_DATATYPE_LATLONALT };           ///< Latitude, longitude, altitude data type.
    using LatLonAlt = SIMCONNECT_DATA_LATLONALT;
    inline constexpr DataType xyz{ SIMCONNECT_DATATYPE_XYZ };                       ///< X, Y, Z data type.
    using XYZ = SIMCONNECT_DATA_XYZ;

    using PitchBankHeading = SIMCONNECT_DATA_PBH;                                   ///< The type representing pitch, bank, and heading.

    inline constexpr DataType max{ SIMCONNECT_DATATYPE_MAX };                       ///< Maximum data type.
}

using DataRequestFlag = unsigned long;        ///< The type used for data request flags.

namespace DataRequestFlags {
    inline constexpr DataRequestFlag defaultFlag{ SIMCONNECT_DATA_REQUEST_FLAG_DEFAULT };    ///< Default data request flag.
    inline constexpr DataRequestFlag whenChanged{ SIMCONNECT_DATA_REQUEST_FLAG_CHANGED };        ///< Changed data request flag.
    inline constexpr DataRequestFlag tagged{ SIMCONNECT_DATA_REQUEST_FLAG_TAGGED };        ///< Tagged data request flag.
}

using DataSetFlag = unsigned long;            ///< The type used for data set flags.

namespace DataSetFlags {
    inline constexpr DataSetFlag defaultFlag{ SIMCONNECT_DATA_SET_FLAG_DEFAULT };    ///< Default data set flag.
    inline constexpr DataSetFlag tagged{ SIMCONNECT_DATA_SET_FLAG_TAGGED };        ///< Tagged data set flag.
}

using EventId = unsigned long;                  ///< The type used for event IDs.
using NotificationGroupId = unsigned long;      ///< The type used for notification group IDs.
using InputGroupId = unsigned long;             ///< The type used for input group IDs.

namespace Events {
    using Priority = unsigned long;              ///< The type used for event priority.

    inline constexpr Priority highestPriority{ SIMCONNECT_GROUP_PRIORITY_HIGHEST };                     ///< The highest event priority.
    inline constexpr Priority highestMaskablePriority{ SIMCONNECT_GROUP_PRIORITY_HIGHEST_MASKABLE };    ///< The highest maskable event priority.
    inline constexpr Priority standardPriority{ SIMCONNECT_GROUP_PRIORITY_STANDARD };                   ///< The standard event priority.
    inline constexpr Priority defaultPriority{ SIMCONNECT_GROUP_PRIORITY_DEFAULT };                     ///< The default event priority.
    inline constexpr Priority lowestPriority{ SIMCONNECT_GROUP_PRIORITY_LOWEST };                       ///< The lowest event priority.

    using State = unsigned long;                         ///< The type used for event state.

    inline constexpr State off{ SIMCONNECT_STATE_OFF };    ///< Event state off.
    inline constexpr State on{ SIMCONNECT_STATE_ON };      ///< Event state on.

    using Flags = unsigned long;                         ///< The type used for event flags.

    inline constexpr Flags defaultFlags{ SIMCONNECT_EVENT_FLAG_DEFAULT };                     ///< Default event flags.
    inline constexpr Flags groupIdIsPriority{ SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY };    ///< Event flag indicating that the group ID is actually a priority.
}

} // namespace SimConnect