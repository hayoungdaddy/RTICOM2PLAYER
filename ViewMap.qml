import QtQuick 2.6
import QtQuick.Controls 1.4
import QtLocation 5.9
import QtPositioning 5.8

Rectangle {
    id: rectangle
    width: 0
    height: 0
    visible: true

    property var staCircle: []
    property var eewStarMarker: "Hello"
    property var aniCircleP: []
    property var aniCircleS: []
    property variant parameters

    signal sendStationIndexSignal(string msg)

    function initializePlugin(pluginParameters)
    {
        var parameters = new Array;
        for(var prop in pluginParameters)
        {
            var parameter = Qt.createQmlObject('import QtLocation 5.9; PluginParameter{ name: "'+ prop + '"; value: "' + pluginParameters[prop] + '"}', map)
            parameters.push(parameter)
        }
        rectangle.parameters = parameters
        map.plugin = Qt.createQmlObject('import QtLocation 5.9; Plugin{ name: "osm"; parameters: rectangle.parameters }', rectangle)
    }

    Map {
        anchors.fill: parent
        id: map
        center: QtPositioning.coordinate(35.80, 127.7)
        zoomLevel: 7.7
        minimumZoomLevel: 7.0
        maximumZoomLevel: 11.9

        MapSliders {
            id: sliders
            z: map.z + 3
            mapSource: map
            edge: Qt.LeftEdge
        }

        onCenterChanged: {
            if(map.center.latitude > 44.5 || map.center.latitude < 30.5 || map.center.longitude > 137.5 || map.center.longitude < 119.5)
                map.center = QtPositioning.coordinate(35.80, 127.7)
        }
    }

    function clearMap()
    {
        map.clearMapItems()
        return true
    }

    function createStaCircle(index, lat, longi, width, color, netsta, opacity)
    {
        staCircle[index].index = index;
        staCircle[index].lati = lat;
        staCircle[index].longi = longi;
        staCircle[index].widthi = width;
        staCircle[index].colors = color;
        staCircle[index].txt = netsta;
        staCircle[index].circleOpacity = opacity;
    }

    function createRealTimeMapObject()
    {
        aniCircleP = Qt.createQmlObject('import QtLocation 5.9; MapCircle { }', map, "dynamic");
        aniCircleP.opacity = 0;
        aniCircleP.border.width = 3;
        aniCircleS = Qt.createQmlObject('import QtLocation 5.9; MapCircle { }', map, "dynamic");
        aniCircleS.opacity = 0;
        aniCircleS.border.width = 3;

        for(var i=0;i<1000;i++)
        {
            staCircle[i] = Qt.createQmlObject('StaCircle { }', map);
            staCircle[i].circleOpacity = 0;
        }

        eewStarMarker = Qt.createQmlObject('EEWStarMarker { }', map);
        eewStarMarker.iconOpacity = 0;

        return true;   
    }

    function createStackedMapObject()
    {
        aniCircleP = Qt.createQmlObject('import QtLocation 5.9; MapCircle { }', map, "dynamic");
        aniCircleP.opacity = 0;
        aniCircleP.border.width = 3;
        aniCircleS = Qt.createQmlObject('import QtLocation 5.9; MapCircle { }', map, "dynamic");
        aniCircleS.opacity = 0;
        aniCircleS.border.width = 3;

        for(var i=0;i<1000;i++)
        {
            staCircle[i] = Qt.createQmlObject('StaCircle { }', map);
            staCircle[i].circleOpacity = 0;
        }

        eewStarMarker = Qt.createQmlObject('EEWStarMarker { }', map);
        eewStarMarker.iconOpacity = 0;

        return true;
    }

    function changeSizeAndColorForStaCircle(index, width, color, opacity)
    {
        staCircle[index].widthi = width;
        staCircle[index].colors = color;
        staCircle[index].circleOpacity = opacity;

        map.addMapItem(staCircle[index]);

        return true;
    }

    function showEEWStarMarker(lat, longi, mag)
    {
        eewStarMarker.coordinate = QtPositioning.coordinate(lat, longi);
        eewStarMarker.eewInfo = "M " + mag;
        eewStarMarker.iconOpacity = 1.0;

        map.addMapItem(eewStarMarker);

        return true;
    }

    function showCircleForAnimation(lat, longi, radP, radS, opa)
    {
        aniCircleP.center = QtPositioning.coordinate(lat, longi);
        aniCircleP.radius = radP;
        aniCircleP.opacity = opa;
        aniCircleP.border.color = 'blue';

        aniCircleS.center = QtPositioning.coordinate(lat, longi);
        aniCircleS.radius = radS;
        aniCircleS.opacity = opa;
        aniCircleS.border.color = 'red';

        map.addMapItem(aniCircleP);
        map.addMapItem(aniCircleS);

        return true;
    }
}
