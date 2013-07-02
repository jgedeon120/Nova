var theDoc = document;
var haveMac = [];
var macsTotal = [];
var nodes = {};
var configIfaces = {};
var $topology = '';
var $dragMe = '';
var $slideInfo = '';
var $profileInfo = '';
var $context = '';
var $modalSplit = '';
var clicked = false;
var selectedProfile = undefined;
var hoverNode = undefined;
var nodeTopology = {};
var nodeCount = {};
var eleCount = 0;
var zoom = {width:50, height:50};
var oldX = 0;
var oldY = 0;
var selected = [];
var profiles = [];
var topoDrag = false;
var noclick = false;
var hoverhold = false;
var group = {type:'DHCP', iface:'eth0'};
var showHideTime = 200;

jQuery.expr[':'].regex = function(elem, index, match) {
  var matchParams = match[3].split(','),
  validLabels = /^(data|css):/,
  attr = {
      method: matchParams[0].match(validLabels) ? 
                  matchParams[0].split(':')[0] : 'attr',
      property: matchParams.shift().replace(validLabels,'')
  },
  regexFlags = 'ig',
  regex = new RegExp(matchParams.join('').replace(/^\s+|\s+$/g,''), regexFlags);
  return regex.test(jQuery(elem)[attr.method](attr.property));
}

function setUpSelects()
{
  var configSelect = $('#configurationSelect');
  for(var i in configurationList)
  {
    if((configurationList[i] != '') && (configurationList[i] != undefined))
    {
      var option = theDoc.createElement('option');
      option.value = configurationList[i];
      option.innerHTML = configurationList[i];
      option.id = configurationList[i];
      if(currentConfig == configurationList[i])
      {
        option.selected = true;
      }
      configSelect.append(option);
    }
  }
}

function showProfileInfo(e)
{
  if(!clicked)
  {
    $profileInfo.stop(false, true).hide();
    var evt = (e ? e : window.event);
    var source = evt.target || evt.srcElement;
    if(typeof now.GetProfile == 'function')
    {
      now.GetProfile($('#name').html(source.innerHTML).html(), function(profile){
        var $vendors = $('#vendors').empty();
        var $portSets = $('#portSets').empty();
        var $interfaces = $('#interfaces').empty();
        
        for(var i in profile.ethernet)
        {
          var option = theDoc.createElement('option');
          option.innerHTML = profile.ethernet[i].vendor;
          $vendors.append(option);
        }
        for(var i in profile.portSets)
        {
          var option = theDoc.createElement('option');
          option.innerHTML = profile.portSets[i].setName;
          $portSets.append(option);
        }
        for(var i in interfaces)
        {
          var option = theDoc.createElement('option');
          option.innerHTML = interfaces[i];
          $interfaces.append(option);
        }
        
        now.GetPortSets(source.innerHTML, function(portsets){
          var $table = $('#profileTableHook');
          $table.empty();
          
          var generateDefaultRow = function(type, sType)
          {
            var tr = theDoc.createElement('tr');
            var td = theDoc.createElement('td');
            $(td).attr('colspan', 2).html('Default ' + sType  + ' Action: ' + udp)
            .toggleClass('right');
            $(tr).append(td);
            $table.append(tr);
          }
          
          var generateGeneralRow = function(pNum, pProtoc, pBehav)
          {
            var tr = theDoc.createElement('tr');
            var td0 = theDoc.createElement('td');
            var td1 = theDoc.createElement('td');
            $(td0).toggleClass('left').html(pNum + '_' + pProtoc);
            $(td1).toggleClass('right').html(pBehav);
            $(tr).append(td0, td1);
            $table.append(tr);
          }
          
          for(var i in portsets)
          {
            if(portsets[i].setName == $portSets.val())
            {
              var udp = portsets[i].UDPBehavior;
              var tcp = portsets[i].TCPBehavior;
              var icmp = portsets[i].ICMPBehavior;
 
              generateDefaultRow(udp, 'UDP');
              generateDefaultRow(tcp, 'TCP');
              generateDefaultRow(icmp, 'ICMP');
              
              for(var j in portsets[i].PortExceptions)
              {
                var num = portsets[i].PortExceptions[j].portNum;
                var proto = portsets[i].PortExceptions[j].protocol;
                var behav = '';
                
                if(portsets[i].PortExceptions[j].behavior == 'script')
                {
                  behav = portsets[i].PortExceptions[j].scriptName;
                }
                else
                {
                  behav = portsets[i].PortExceptions[j].behavior;
                }
                
                generateGeneralRow(num, proto, behav);
              }
              break;
            }
          }
          var offset = $('#tabDock').outerHeight() + $('#headContainer').outerHeight() + 2;
          var pointerCss = {top:($(source).position().top - ($profileInfo.outerHeight() / 2) + offset),
                            left:($(source).position().left + $(source).outerWidth() + 10)};

          $profileInfo.css(pointerCss).stop(false, true).fadeIn(400);
        });
      });
    }
    else
    {
      console.log('Could not get profile data: No connection to server.');
    }
  }
}

function hideProfileInfo(e)
{
  if(!clicked)
  {
    if(e == undefined)
    {
      $profileInfo.stop(false, true).fadeOut(100);
      return;
    }
    var evt = (e ? e : window.event);
    var source = evt.target || evt.srcElement;
    if($('#profilesHook').find(source) && source.tagName == 'TD')
    {
      return;
    }
    else
    {
      $profileInfo.stop(false, true).fadeOut(100);
    }
  }
}

function profileClicked(e)
{
  var evt = (e ? e : window.event);
  var source = evt.target || evt.srcElement;
  if(clicked && (source.innerHTML != selectedProfile))
  {
    return;
  }
  else if(clicked && (source.innerHTML == selectedProfile))
  {
    $(source).css('background-color', '');
    selectedProfile = undefined;
    clicked = false;
    return;
  }
  $(source).css('background-color', '#E8A02F');
  selectedProfile = source.innerHTML;
  clicked = true;
}

function updateDragHeader()
{
  var set = $('#nodeNumber').val();
  $('#setNumNodes').html(set);
}

function clearProfileSelected()
{
  $('#profilesHook').find('td').css('background-color', '');
  selectedProfile = undefined;
}

function repopulateNodeCanvas(cb)
{
  currentConfig = $('#configurationSelect').val();
  nodeTopology = {};
  $('.canvasElement').each(function(){
    $(this).remove();
  });
  $('div.canvas div:regex(id, ^[0-9]?osi[blt]?$)').each(function(){
    $(this).remove();
  });
  if(typeof now.GetConfigSummary == 'function')
  { 
    now.GetConfigSummary(currentConfig, function(scriptProfileBindings, profileObj){
      var profHook = $('#profilesHook');
      profHook.empty();
      
      for(var i in profileObj)
      {
        var profile = {};
        profile.name = profileObj[i].name;
        profiles.push(profile.name);
        profile.parent = profileObj[i].parent;
        profile.children = [];
        var append = theDoc.createElement('tr');
        var td0 = theDoc.createElement('td');
        $(td0).html(profile.name).click(function(e){profileClicked(e)});
        append.appendChild(td0);
        profHook.append(append);
      }
      
      profHook.find('td').first().toggleClass('first');
      if(profHook.find('td').size() >= 3)
      {
        profHook.find('td').slice(1, -1).toggleClass('middle');
      }
      profHook.find('td').last().toggleClass('last');
      
      cb && cb();
    });
  }
}

function saveSelectedParameters()
{
  clicked = false;
  var dragMeCss = {border:'1px solid black',
                   opacity:'1'};
  $dragMe.draggable('enable').css(dragMeCss);
  hideProfileInfo();
  var profileObject = {};
  profileObject.pfile = selectedProfile;
  placeBackgroundImage($dragMe, selectedProfile);
  profileObject.enabled = true;
  profileObject.count = $('#nodeNumber').val();
  profileObject.portset = $('#portSets').val();
  profileObject.vendor = $('#vendors').val();
  profileObject.iface = $('#interfaces').val();
  profileObject.ip = $('#allocationType').val();
  
  if(profileObject.ip != 'DHCP')
  {
    profileObject.ip = {ip1:$('#ip1').val(), 
                        ip2:$('#ip2').val(), 
                        ip3:$('#ip3').val(), 
                        ip4:$('#ip4').val()};
  }
  
  nodeTopology[eleCount] = profileObject;
}

function showIpBlock()
{
  if($('#allocationType').val() != 'DHCP')
  {
    $('#ipBlock').css('display', 'block');
  }
  else
  {
    $('#ipBlock').css('display', 'none');
  }
}

function adjustColumns()
{
  var $sameHeightDivs = $('.sameHeight');
  var maxHeight = 0;
  $sameHeightDivs.each(function(){
    maxHeight = Math.max(maxHeight, $(this).outerHeight());
  });
  $sameHeightDivs.css({height: maxHeight + 'px'});
}

var fadeEleIn = true;

function handleCanvasEleHover(e, ui)
{
  if(e.type == 'mouseenter')
  {
    var evt = (e ? e : window.event);
    var source = evt.target || evt.srcElement;
    if(source.id.indexOf('delete') != -1 || hoverhold)
    {
      return;
    }
    var titleCss = {'font-weight':'bold', 'font-size':'18px'};
    var $hName = $('#hoverName').css(titleCss).html($(source).data('profile'));
    var $hPortset = $('#hoverPortset').html('');
    var $hVendor = $('#hoverVendor').html('');
    var $hIface = $('#hoverInterface').html('');
    var $hAddress = $('#hoverAddress').html('');
    
    var id = source.id.toString();
    
    if(nodeTopology[id] != undefined)
    {
      $hPortset.html(nodeTopology[id].portset);
      $hVendor.html(nodeTopology[id].vendor);
      $hIface.html(nodeTopology[id].iface);
      $('#changeCount').val(nodeTopology[id].count);
      
      if(nodeTopology[id].ip != 'DHCP')
      {
        $hAddress.html('IP Range starting at: ' + nodeTopology[id].ip);
      }
      else
      {
        $hAddress.html('DHCP');
      }
    }
    else
    {
      if($(source).data('mac') != undefined)
      {
        var splitMac = $(source).data('mac');
        var mac = splitMac.split(',')[0];
        for(var i in nodeList)
        {
          if(nodeList[i].mac == mac)
          {
            $hPortset.html(nodeList[i].portset);
            $hVendor.html(nodeList[i].vendor);
            $hIface.html(nodeList[i].interface);
            $('#changeCount').val(nodeList[i].count);
            if(nodeList[i].ip != 'DHCP')
            {
              $hAddress.html('IP Range starting at: ' + nodeList[i].ip);
            }
            else
            {
              $hAddress.html(nodeList[i].ip);
            }
            break;
          }
        }
      }
    }
    
    if(fadeEleIn || !$slideInfo.is(':hidden'))
    {
      $slideInfo.stop(false, true).show('slide', {direction:'right'}, showHideTime);
      fadeEleIn = false;
    }
  }
  else if(e.type == 'mouseleave')
  {
    
    if(!hoverhold)
    {
      $slideInfo.stop(false, true).hide('slide', {direction:'right'}, showHideTime, function(){
        $slideInfo.removeAttr('style').hide().css({height:$topology.css('height')});
        fadeEleIn = true;
      });
    }
  }
  else if(e.type == 'mouseover')
  {
    var evt = (e ? e : window.event);
    var source = evt.target || evt.srcElement;
    if(source.id.indexOf('delete') == -1)
    {
      $slideInfo.stop(false, true).show('slide', {direction:'right'}, showHideTime);
    }
  }
  else if(e.type == 'drag')
  {
    $slideInfo.hide('slide', {direction:'right'}, showHideTime);
    fadeEleIn = true;
    $contextMenu.fadeOut(100);
    var evt = (e ? e : window.event);
    var source = evt.target || evt.srcElement;
    var x = ui.helper.offset().left - $topology.offset().left;
    var dx = x - oldX;
    var y = ui.helper.offset().top - $topology.offset().top;
    var dy = y - oldY;
    
    if($(source).attr('class') != undefined && $(source).attr('class').indexOf('ui-selected') != -1)
    {
      $('.ui-selected').not($(source)).each(function(){
        var relX = parseInt(dx) + parseInt($(this).css('left'));
        var relY = parseInt(dy) + parseInt($(this).css('top'));
        var adjustPos = {left:relX, top:relY};
        $(this).css(adjustPos);    
      });
      handleOffscreenIndicators();
    }
    oldX = x;
    oldY = y;
  }
  else if(e.type == 'dragstop')
  {
    hoverhold = false;
    $slideInfo.stop(false, true).show('slide', {direction:'right'}, showHideTime);
  }
}

function canvasElementOnclick(e)
{
  if(!noclick)
  {
    e.stopImmediatePropagation();
    var evt = (e ? e : window.event);
    var source = evt.target || evt.srcElement;
   
    if(source.tagName != 'DIV')
    {
      source = source.parentNode;
    }
    if($(source).attr('class').indexOf('ui-selected') != -1)
    {
      $(source).css('border', '').removeClass('ui-selected');
      for(var i in selected)
      {
        if(selected[i] == source)
        {
          delete selected[i];
        }
      }
      hoverNode = undefined;
      hoverhold = false;
    }
    else
    {
      $(source).css('border', '1px ridge red').addClass('ui-selected');
      selected.push(source);
      $slideInfo.stop(false, true).show('slide', {direction:'right'}, showHideTime);
      hoverNode = source.id;
      hoverhold = true;
    }
  }
  else
  {
    noclick = false;
  }
}

function changeTab(e)
{
  now.WriteWysiwygTopology(nodeTopology, function(){
    $('#b' + $topology.attr('id')).removeClass('selectedWysiwygTab').addClass('unselectedWysiwygTab');
    clearTopoListeners($topology);
    $('#b' + e).removeClass('unselectedWysiwygTab').addClass('selectedWysiwygTab');
    $topology.css({'display':'none'});
    $topology = $('#' + e);
    appendTopoListeners($topology);
    $topology.css({'display':'block'});
    handleOffscreenIndicators();
    $('#saveMessage').stop(false, true).show().delay(1000).fadeOut('slow');
  });
}

function deleteCanvasEleClick(e, source)
{
  e.stopImmediatePropagation();
  macsToRemove = nodeTopology[source].mac.split(',');
  delete nodeTopology[source];
  now.deleteNodes(macsToRemove);
  $('#' + source).remove();
  if(hoverNode == source || hoverNode == undefined)
  {
    $slideInfo.stop(false, true).hide('slide', {direction:'right'}, showHideTime, function(){
      $slideInfo.removeAttr('style').hide().css({height:$topology.css('height')});
      fadeEleIn = true;
    });
  }
}

function addNodeToCanvas(nodeObj, type, left, top, xEleCount, yEleCount, splitData)
{
  var div = theDoc.createElement('div');
  
  var x = 0;
  var y = 0;
  
  if((left != undefined) && (top != undefined))
  {
    x = left;
    y = top;
  }
  
  $(div)
  .click(function(e){
    canvasElementOnclick(e);
  })
  .draggable({tolerance: 'pointer',
    cursorAt: {top:25,left:15},
    cursor: 'move',
    containment: ('#nodeCanvas' + nodeObj.iface),
    revert: 'invalid'});

  if((xEleCount != undefined) && (yEleCount != undefined))
  {
    y = (yEleCount * zoom.height);
    x = (xEleCount * zoom.width);
    
    if(x > $topology.outerWidth() && !$topology.is(':hidden'))
    {
      yEleCount += 1;
      xEleCount = 0;
      y = (yEleCount * zoom.height);
      x = (xEleCount * zoom.width);
    }
  }
  
  var prepopCss = {position:'absolute', left:(x + 'px'), top:(y + 'px')};
  var bgSize = zoom.width + 'px ' + zoom.height + 'px';
  $(div).data('profile', nodeObj.pfile)
        .css(prepopCss)
        .css(zoom)
        .attr('class', 'canvasElement ui-draggable');
  if((nodeObj.mac != undefined) && (type == 'prepopDHCP' || type == 'prepopStatic' || type == 'prepop'))
  {
    $(div).data('mac', nodeObj.mac)
          .data('preexisting', true);
  }
  else if(type == 'split')
  {
    $(div).data('mac', splitData.splNodeMacs);
  }
  div.id = eleCount;
  
  placeBackgroundImage($(div), nodeObj.pfile);
  setCanvasElementEventHandlers(div);
  
  var text = theDoc.createElement('label');
  text.innerHTML = nodeObj.count;
  text.style.fontSize = '18px';
  text.style.cssFloat = 'left';
  text.style.borderRight = '1px solid black';
  text.style.backgroundColor = '#F1CC91';
  text.id = eleCount + 'text';
  div.appendChild(text);
  
  var deleteMe = theDoc.createElement('label');
  deleteMe.innerHTML = 'X';
  deleteMe.style.fontSize = '18px';
  deleteMe.style.cssFloat = 'right';
  deleteMe.style.borderLeft = '1px solid black';
  deleteMe.style.backgroundColor = 'red';
  deleteMe.setAttribute('onclick', 'deleteCanvasEleClick(event, ' + eleCount + ')');
  deleteMe.id = eleCount + 'delete';
  div.appendChild(deleteMe);
  
  if(type == 'drop')
  {
    var ipType = '';
    var ip1, ip2, ip3, ip4 = '0';
    if(nodeTopology[eleCount].ip != 'DHCP')
    {
      ip1 = nodeTopology[eleCount].ip.ip1;
      ip2 = nodeTopology[eleCount].ip.ip2;
      ip3 = nodeTopology[eleCount].ip.ip3;
      ip4 = nodeTopology[eleCount].ip.ip4;
    }
    else
    {
      ipType = 'DHCP';
    }
    var profile = nodeTopology[eleCount].pfile;
    var portset = nodeTopology[eleCount].portset.toString();
    var vendor = nodeTopology[eleCount].vendor;
    var ethInterface = nodeTopology[eleCount].iface;
    var count = nodeTopology[eleCount].count.toString();
    if((typeof now.createHoneydNodes == 'function') && (typeof now.GetNodes == 'function'))
    {
      now.createHoneydNodes(ipType, ip1, ip2, ip3, ip4, profile, portset, vendor, ethInterface, count);

      nodes = [];
      var setMAC = [];

      now.GetNodes(function(nodesList){
        nodes = nodesList;
        nodeList = nodesList;
        var add = true;
        for(var i in nodes)
        {
          add = true;
          for(var j in haveMac)
          {
            if(haveMac[j] == nodes[i].mac)
            {
              add = false;
              break;
            }
          }
          if(add)
          {
            haveMac.push(nodes[i].mac);
            setMAC.push(nodes[i].mac);
          } 
        }
        $(div).data('mac', setMAC.join());
        nodeTopology[eleCount].mac = setMAC.join();
        nodeTopology[eleCount].coordinates = {x:x, y:y};
        if($topology != undefined && $topology != '')
        {
          $topology.append(div);
        }

        eleCount++;
      });
    }
    else
    {
      alert('Could not create honeyd nodes: No connection to server');
    }
  }
  else if(type == 'prepopDHCP')
  {
    var macSplit = nodeObj.mac.split(',')[0];
    if(nodeList.length != undefined)
    {
      for(var j in nodeList)
      {
        if(nodeList[j].mac == macSplit)
        {
          nodeTopology[eleCount] = {};
          nodeTopology[eleCount].enabled = true;
          nodeTopology[eleCount].iface = nodeList[j].interface;
          nodeTopology[eleCount].count = nodeObj.count.toString();
          nodeTopology[eleCount].portset = nodeList[j].portset.toString();
          nodeTopology[eleCount].vendor = nodeList[j].vendor;
          nodeTopology[eleCount].ip = nodeList[j].ip;
          nodeTopology[eleCount].pfile = nodeObj.pfile;
          nodeTopology[eleCount].mac = nodeObj.mac;
          nodeTopology[eleCount].coordinates = {x:x, y:y};
          $('#nodeCanvas' + nodeList[j].interface).append(div);
          break;
        }
      }
    }
    else
    {
      nodeTopology[eleCount] = {};
      nodeTopology[eleCount].enabled = true;
      nodeTopology[eleCount].iface = nodeObj.iface;
      nodeTopology[eleCount].count = nodeObj.count.toString();
      nodeTopology[eleCount].portset = nodeObj.portset.toString();
      nodeTopology[eleCount].vendor = nodeObj.vendor;
      nodeTopology[eleCount].ip = nodeObj.ip;
      nodeTopology[eleCount].pfile = nodeObj.pfile;
      nodeTopology[eleCount].mac = nodeObj.mac;
      nodeTopology[eleCount].coordinates = {x:x, y:y};
      $('#nodeCanvas' + nodeObj.iface).append(div);
    }
    
    eleCount++;
  }
  else if(type == 'prepopStatic')
  {
    var macSplit = nodeObj.mac.split(',')[0];
    if(nodeList.length != undefined)
    {
      for(var j in nodeList)
      {
        if(nodeList[j].mac == macSplit)
        {
          nodeTopology[eleCount] = {};
          nodeTopology[eleCount].enabled = true;
          nodeTopology[eleCount].iface = nodeList[j].interface;
          nodeTopology[eleCount].count = nodeObj.count.toString();
          nodeTopology[eleCount].portset = nodeList[j].portset.toString();
          nodeTopology[eleCount].vendor = nodeList[j].vendor;
          nodeTopology[eleCount].ip = nodeList[j].ip;
          nodeTopology[eleCount].pfile = nodeObj.pfile;
          nodeTopology[eleCount].mac = nodeObj.mac;
          nodeTopology[eleCount].coordinates = {x:x, y:y};
          $('#nodeCanvasstatic').append(div);
          break;
        }
      }
    }
    else
    {
      nodeTopology[eleCount] = {};
      nodeTopology[eleCount].enabled = true;
      nodeTopology[eleCount].iface = nodeObj.iface;
      nodeTopology[eleCount].count = nodeObj.count.toString();
      nodeTopology[eleCount].portset = nodeObj.portset.toString();
      nodeTopology[eleCount].vendor = nodeObj.vendor;
      nodeTopology[eleCount].ip = nodeObj.ip;
      nodeTopology[eleCount].pfile = nodeObj.pfile;
      nodeTopology[eleCount].mac = nodeObj.mac;
      nodeTopology[eleCount].coordinates = {x:x, y:y};
      $('#nodeCanvasstatic').append(div);
    }
    
    eleCount++;
  }
  else if(type == 'prepop')
  {
    var macSplit = nodeObj.mac.split(',')[0];
    for(var j in nodeList)
    {
      if(nodeList[j].mac == macSplit)
      {
        nodeTopology[eleCount] = {};
        nodeTopology[eleCount].enabled = true;
        nodeTopology[eleCount].iface = nodeList[j].interface;
        nodeTopology[eleCount].count = nodeObj.count.toString();
        nodeTopology[eleCount].portset = nodeList[j].portset.toString();
        nodeTopology[eleCount].vendor = nodeList[j].vendor;
        nodeTopology[eleCount].ip = nodeList[j].ip;
        nodeTopology[eleCount].pfile = nodeObj.pfile;
        nodeTopology[eleCount].mac = nodeObj.mac;
        nodeTopology[eleCount].coordinates = {x:x, y:y};
        if(nodeList[j].ip == 'DHCP')
        {
          $('#nodeCanvas' + nodeList[j].interface).append(div);
        }
        else
        {
          $('#nodeCanvasstatic').append(div);
        }
        break;
      }
    }
    
    eleCount++;
  }
  else if(type == 'split')
  {
    $(splitData.src).data('mac', splitData.srcNodeMacs);
    $('#' + splitData.src.id + 'text').html(splitData.srcCount);
    nodeTopology[splitData.src.id].count = splitData.srcCount;
    nodeTopology[splitData.src.id].mac = splitData.srcNodeMacs;
    
    var testMac = splitData.splNodeMacs.split(',')[0];
    for(var j in nodes)
    {
      if(nodes[j].mac == testMac)
      {
        nodeTopology[eleCount] = {};
        nodeTopology[eleCount].enabled = true;
        nodeTopology[eleCount].iface = nodes[j].interface;
        nodeTopology[eleCount].count = splitData.splCount;
        nodeTopology[eleCount].portset = nodes[j].portset.toString();
        nodeTopology[eleCount].vendor = nodes[j].vendor;
        nodeTopology[eleCount].ip = nodes[j].ip;
        nodeTopology[eleCount].pfile = nodeObj.pfile;
        nodeTopology[eleCount].mac = splitData.splNodeMacs;
        nodeTopology[eleCount].coordinates = {x:x, y:y};
        break;
      }
    }
    $topology.append(div);
    
    eleCount++;
  }
  else
  {
    return undefined;
  }
  if(xEleCount != undefined)
  {
    xEleCount++;
    return {xCount:xEleCount, yCount:yEleCount};
  }
  return undefined;
}

function generateTab(iface)
{
  if($('#bnodeCanvas' + iface).length != 0)
  {
    return;
  }
  var tab = theDoc.createElement('button');
  tab.className = 'unselectedWysiwygTab';
  tab.id = 'bnodeCanvas' + iface;
  tab.setAttribute('onclick', 'changeTab("nodeCanvas' + iface + '")');
  tab.innerHTML = iface;
  if($('.selectedWysiwygTab').length == 0)
  {
    tab.className = 'selectedWysiwygTab';
  }
  $('#tabDock').append(tab);
}

function regenTabDock()
{
  $('#tabDock').empty();
  if($('#0').length != 0)
  {
    for(var i in configIfaces[currentConfig])
    {
      generateTab(configIfaces[currentConfig][i]);
    }
  }
}

function prepopulateCanvasWithNodes(cb)
{
  // TODO: Need to check for MAC mismatch between TOPO file and 
  // pulled info, if using a topo file.
  var xEleCount = 0;
  var yEleCount = 0;
  var eleCountUpdate = {};

  if(configIfaces[currentConfig] == undefined)
  {
    configIfaces[currentConfig] = [];
  }
  
  if(typeof now.ReadWysiwygTopology == 'function')
  {
    now.ReadWysiwygTopology(function(topo){
      if((topo != undefined) && (topo.length > 0))
      {
        for(var i in topo)
        {
          if(topo[i].ip == 'DHCP')
          {
            if(configIfaces[currentConfig].indexOf(topo[i].iface) == -1)
            {
              configIfaces[currentConfig].push(topo[i].iface);  
            }
            if($('#' + topo[i].iface + 'tab').length == 0 
            && macsTotal.indexOf(topo[i].mac) != -1
            && profiles.indexOf(topo[i].pfile) != -1)
            {
              var tabDiv = theDoc.createElement('div');
              tabDiv.id = topo[i].iface + 'tab';
              tabDiv.setAttribute('onclick', 'clearSelectedCanvas()');
              var canvasDiv = theDoc.createElement('div');
              canvasDiv.id = 'nodeCanvas' + topo[i].iface;
              canvasDiv.style.display = 'none';
              canvasDiv.className = 'canvas sameHeight';
              tabDiv.appendChild(canvasDiv);
              $topoHook.append(tabDiv);
              if($topology == '')
              {
                canvasDiv.style.display = 'block';
                $topology = $(canvasDiv);
              }
            }
            addNodeToCanvas(topo[i], 'prepopDHCP', topo[i].coordinates.x, topo[i].coordinates.y); 
          }
          else
          {
            if(configIfaces[currentConfig].indexOf('static') == -1)
            {
              configIfaces[currentConfig].push('static');  
            }
            if($('#statictab').length == 0
            && macsTotal.indexOf(topo[i].mac) != -1
            && profiles.indexOf(topo[i].pfile) != -1)
            {
              var tabDiv = theDoc.createElement('div');
              tabDiv.id = 'statictab';
              tabDiv.setAttribute('onclick', 'clearSelectedCanvas()');
              var canvasDiv = theDoc.createElement('div');
              canvasDiv.id = 'nodeCanvasstatic';
              canvasDiv.style.display = 'none';
              canvasDiv.className = 'canvas sameHeight';
              tabDiv.appendChild(canvasDiv);
              $topoHook.append(tabDiv);
              if($topology == '')
              {
                canvasDiv.style.display = 'block';
                $topology = $(canvasDiv);
              }
            }
            addNodeToCanvas(topo[i], 'prepopStatic', topo[i].coordinates.x, topo[i].coordinates.y);
          }
        }
        handleOffscreenIndicators();
        regenTabDock();
        cb && cb();
      }
      else
      {
        if(nodeList.length == undefined || nodeList.length == 0)
        {
          now.GetNodes(function(nodesList){
            nodeList = nodesList;
            for(var i in nodeList)
            {
              if(nodeList[i].ip == 'DHCP')
              {
                if(configIfaces[currentConfig].indexOf(nodeList[i].interface) == -1)
                {
                  configIfaces[currentConfig].push(nodeList[i].interface);  
                }
                if($('#' + nodeList[i].interface + 'tab').length == 0)
                {
                  var tabDiv = theDoc.createElement('div');
                  tabDiv.id = nodeList[i].interface + 'tab';
                  tabDiv.setAttribute('onclick', 'clearSelectedCanvas()');
                  var canvasDiv = theDoc.createElement('div');
                  canvasDiv.id = 'nodeCanvas' + nodeList[i].interface;
                  canvasDiv.style.display = 'none';
                  canvasDiv.className = 'canvas sameHeight';
                  tabDiv.appendChild(canvasDiv);
                  $topoHook.append(tabDiv);
                  if($topology == '')
                  {
                    canvasDiv.style.display = 'block';
                    $topology = $(canvasDiv);
                  }
                }
              }
              else
              {
                if(configIfaces[currentConfig].indexOf('static') == -1)
                {
                  configIfaces[currentConfig].push('static');  
                }
                if($('#statictab').length == 0)
                {
                  var tabDiv = theDoc.createElement('div');
                  tabDiv.id = 'statictab';
                  tabDiv.setAttribute('onclick', 'clearSelectedCanvas()');
                  var canvasDiv = theDoc.createElement('div');
                  canvasDiv.id = 'nodeCanvasstatic';
                  canvasDiv.style.display = 'none';
                  canvasDiv.className = 'canvas sameHeight';
                  tabDiv.appendChild(canvasDiv);
                  $topoHook.append(tabDiv);
                  if($topology == '')
                  {
                    canvasDiv.style.display = 'block';
                    $topology = $(canvasDiv);
                  }
                }
              }
              haveMac.push(nodeList[i].mac);
              var key = nodeList[i].pfile + nodeList[i].portset + nodeList[i].interface + nodeList[i].ip;
              if(nodeCount[key] == undefined)
              {
                nodeCount[key] = {};
                nodeCount[key].count = 1;
                nodeCount[key].mac = nodeList[i].mac;
                nodeCount[key].pfile = nodeList[i].pfile;
              }
              else
              {
                nodeCount[key].count++;
                nodeCount[key].mac += ',' + nodeList[i].mac;
              }
            }
            for(var i in nodeCount)
            {
              eleCountUpdate = addNodeToCanvas(nodeCount[i], 'prepop', undefined, undefined, xEleCount, yEleCount);
              if(eleCountUpdate != undefined)
              {
                xEleCount = eleCountUpdate.xCount;
                yEleCount = eleCountUpdate.yCount;
              }
              else
              {
                eleCountUpdate = {};
              }
            }
            nodeCount = {};
            handleOffscreenIndicators();
            regenTabDock();
            cb && cb();
          });
        }
        else
        {
          for(var i in nodeList)
          {
            haveMac.push(nodeList[i].mac);
            if(nodeList[i].ip == 'DHCP')
            {
              if(configIfaces[currentConfig].indexOf(nodeList[i].interface) == -1)
              {
                configIfaces[currentConfig].push(nodeList[i].interface);  
              }
              if($('#' + nodeList[i].interface + 'tab').length == 0)
              {
                var tabDiv = theDoc.createElement('div');
                tabDiv.id = nodeList[i].interface + 'tab';
                tabDiv.setAttribute('onclick', 'clearSelectedCanvas()');
                var canvasDiv = theDoc.createElement('div');
                canvasDiv.id = 'nodeCanvas' + nodeList[i].interface;
                canvasDiv.style.display = 'none';
                canvasDiv.className = 'canvas sameHeight';
                tabDiv.appendChild(canvasDiv);
                $topoHook.append(tabDiv);
                if($topology == '')
                {
                  canvasDiv.style.display = 'block';
                  $topology = $(canvasDiv);
                }
              }
            }
            else
            {
              if(configIfaces[currentConfig].indexOf('static') == -1)
              {
                configIfaces[currentConfig].push('static');  
              }
              if($('#statictab').length == 0)
              {
                var tabDiv = theDoc.createElement('div');
                tabDiv.id = 'statictab';
                tabDiv.setAttribute('onclick', 'clearSelectedCanvas()');
                var canvasDiv = theDoc.createElement('div');
                canvasDiv.id = 'nodeCanvasstatic';
                canvasDiv.style.display = 'none';
                canvasDiv.className = 'canvas sameHeight';
                tabDiv.appendChild(canvasDiv);
                $topoHook.append(tabDiv);
                if($topology == '')
                {
                  canvasDiv.style.display = 'block';
                  $topology = $(canvasDiv);
                }                
              }
            }
            var key = nodeList[i].pfile + nodeList[i].portset + nodeList[i].interface + nodeList[i].ip;
            if(nodeCount[key] == undefined)
            {
              nodeCount[key] = {};
              nodeCount[key].count = 1;
              nodeCount[key].mac = nodeList[i].mac;
              nodeCount[key].pfile = nodeList[i].pfile;
            }
            else
            {
              nodeCount[key].count++;
              nodeCount[key].mac += ',' + nodeList[i].mac;
            }
          }
          for(var i in nodeCount)
          {
            eleCountUpdate = addNodeToCanvas(nodeCount[i], 'prepop', undefined, undefined, xEleCount, yEleCount);
            if(eleCountUpdate != undefined)
            {
              xEleCount = eleCountUpdate.xCount;
              yEleCount = eleCountUpdate.yCount;
            }
            else
            {
              eleCountUpdate = {};
            }
          }
          nodeCount = {};
          handleOffscreenIndicators();
          regenTabDock();
          cb && cb();
        }
      }
    });
  }
  else
  {
    console.log('Could not read saved topology file: No connection to server');
  }
}

function placeBackgroundImage(ele, profile)
{
  var cssObj = {'background-repeat':'no-repeat',
                'background-origin':'center',
                'background-color':'#E4F2FF',
                'background-size':(zoom.width + 'px ' + zoom.height + 'px'),
                'background-image':''};
  if(profile == undefined)
  {
    cssObj['background-image'] = 'url("images/wysiwyg/unknown_75.png")';
    ele.css(cssObj);
    return;
  }
  if(typeof now.GetProfile == 'function')
  {
    now.GetProfile(profile, function(pfile){
      if(pfile == null || pfile == undefined)
      {
        return;
      }
      var pers = pfile.personality;
      if(pers.indexOf('Windows') != -1)
      {
        cssObj['background-image'] = 'url("images/wysiwyg/win_75.png")';
        ele.css(cssObj);
      }
      else if(pers.indexOf('OpenBSD') != -1)
      {
        cssObj['background-image'] = 'url("images/wysiwyg/openbsd_75.png")';
        ele.css(cssObj);
      }
      else if(pers.indexOf('Linux') != -1)
      {
        cssObj['background-image'] = 'url("images/wysiwyg/linux_75.png")';
        ele.css(cssObj);
      }
      else if(pers.indexOf('Mac OS X') != -1)
      {
        cssObj['background-image'] = 'url("images/wysiwyg/macosx_75.png")';
        ele.css(cssObj);
      }
      else
      {
        cssObj['background-image'] = 'url("images/wysiwyg/unknown_75.png")';
        ele.css(cssObj);
      }
    });
  }
  else
  {
    cssObj['background-image'] = 'url("images/wysiwyg/unknown_75.png")';
    ele.css(cssObj);
  }
}

function handleOffscreenIndicators()
{
  if($topology == undefined || $topology == '')
  {
    return;
  }
  var width = $topology.outerWidth();
  var height = $topology.outerHeight();
  $('.canvasElement').each(function(){
    if((parseInt($(this).css('left')) + zoom.width <= 0) 
    && (parseInt($(this).css('top')) < -zoom.height))
    {
      if(theDoc.getElementById(this.id + 'ositl') == undefined)
      {
        var div = theDoc.createElement('div');
        var pos = {left:-20, top:-20};
        div.className = 'offscreenIndicatorCornerTL';
        div.id = this.id + 'ositl';
        $(div).css(pos);
        $('#' + this.id + 'osil').remove();
        $('#' + this.id + 'osit').remove();
        $topology.append(div);
      }
    }
    else if((parseInt($(this).css('left')) + zoom.width <= 0) 
    && (parseInt($(this).css('top')) > height))
    {
      if(theDoc.getElementById(this.id + 'osibl') == undefined)
      {
        var div = theDoc.createElement('div');
        var pos = {left:-20, top:height - 25};
        div.className = 'offscreenIndicatorCornerBL';
        div.id = this.id + 'osibl';
        $(div).css(pos);
        $('#' + this.id + 'osil').remove();
        $('#' + this.id + 'osib').remove();
        $topology.append(div);
      }
    }
    else if((parseInt($(this).css('left')) > width) 
    && (parseInt($(this).css('top')) < -zoom.height))
    {
      if(theDoc.getElementById(this.id + 'ositr') == undefined)
      {
        var div = theDoc.createElement('div');
        var pos = {left:width - 50, top:10};
        div.className = 'offscreenIndicatorCornerTR';
        div.id = this.id + 'ositr';
        $(div).css(pos);
        $('#' + this.id + 'osir').remove();
        $('#' + this.id + 'osit').remove();
        $topology.append(div);
      }
    }
    else if((parseInt($(this).css('left')) > width) 
    && (parseInt($(this).css('top')) > height))
    {
      if(theDoc.getElementById(this.id + 'osibr') == undefined)
      {
        var div = theDoc.createElement('div');
        var pos = {left:width - 65, top:height - 25};
        div.className = 'offscreenIndicatorCornerBR';
        div.id = this.id + 'osibr';
        $(div).css(pos);
        $('#' + this.id + 'osir').remove();
        $('#' + this.id + 'osib').remove();
        $topology.append(div);
      }
    }
    else if(parseInt($(this).css('left')) > width)
    {
      if(theDoc.getElementById(this.id + 'osir') == undefined)
      {
        var div = theDoc.createElement('div');
        var pos = {left:width, top:$(this).css('top')};
        div.className = 'offscreenIndicatorRight';
        div.id = this.id + 'osir';
        $(div).css(pos);
        $topology.append(div);
      }
      else
      {
        var pos = {top:$(this).css('top')};
        $('#' + this.id + 'osir').css(pos);
      }
    }
    else if(parseInt($(this).css('left')) + (zoom.width / 2) <= 0)
    {
      if(theDoc.getElementById(this.id + 'osil') == undefined)
      {
        var div = theDoc.createElement('div');
        var pos = {left:-20, top:$(this).css('top')};
        div.className = 'offscreenIndicatorLeft';
        div.id = this.id + 'osil';
        $(div).css(pos);
        $topology.append(div);
      }
      else
      {
        var pos = {top:$(this).css('top')};
        $('#' + this.id + 'osil').css(pos);
      }
    }
    else if(parseInt($(this).css('top')) < -zoom.height)
    {
      if(theDoc.getElementById(this.id + 'osit') == undefined)
      {
        var div = theDoc.createElement('div');
        var pos = {left:$(this).css('left'), top:0};
        div.className = 'offscreenIndicatorTop';
        div.id = this.id + 'osit';
        $(div).css(pos);
        $topology.append(div);
      }
      else
      {
        var pos = {left:$(this).css('left')};
        $('#' + this.id + 'osit').css(pos);
      }
    }
    else if(parseInt($(this).css('top')) > height)
    {
      if(theDoc.getElementById(this.id + 'osib') == undefined)
      {
        var div = theDoc.createElement('div');
        var pos = {left:$(this).css('left'), top:height - 45};
        div.className = 'offscreenIndicatorBottom';
        div.id = this.id + 'osib';
        $(div).css(pos);
        $topology.append(div);
      }
      else
      {
        var pos = {left:$(this).css('left')};
        $('#' + this.id + 'osib').css(pos);
      }
    }
    if(parseInt($(this).css('top')) > 0)
    {
      $('#' + this.id + 'osit').remove();
      $('#' + this.id + 'ositl').remove();
      $('#' + this.id + 'ositr').remove();
    }
    if(parseInt($(this).css('top')) < height)
    {
      $('#' + this.id + 'osib').remove();
      $('#' + this.id + 'osibl').remove();
      $('#' + this.id + 'osibr').remove(); 
    }
    if(parseInt($(this).css('left')) < width)
    {
      $('#' + this.id + 'osir').remove();
      $('#' + this.id + 'ositr').remove();
      $('#' + this.id + 'osibr').remove();
    }
    if(parseInt($(this).css('left')) >= 0)
    {
      $('#' + this.id + 'osil').remove();
      $('#' + this.id + 'ositl').remove();
      $('#' + this.id + 'osibl').remove();
    }
  });
}

function setCanvasElementEventHandlers(ele)
{
  $(ele)
    .on('dragstop', function(e){
      handleCanvasEleHover(e);
      $('.ui-selected').css('border', '').removeClass('ui-selected');
      $(this).on('mouseenter', function(e){
        e.stopImmediatePropagation();
        handleCanvasEleHover(e);
      })
      .on('mouseleave', function(e){
        e.stopImmediatePropagation();
        handleCanvasEleHover(e);
      })
    })
    .on('mouseenter', function(e){
      e.stopImmediatePropagation();
      handleCanvasEleHover(e);
    })
    .on('mouseleave', function(e){
      e.stopImmediatePropagation();
      handleCanvasEleHover(e);
    })
    .on('drag', function(e, ui){
      e.stopImmediatePropagation();
      handleCanvasEleHover(e, ui);
      $(this).off('mouseleave');
      $(this).off('mouseenter');
    })
    .on('dragstart', function(e, ui){
      oldX = ui.helper.offset().left - $topology.offset().left;
      oldY = ui.helper.offset().top - $topology.offset().top;
      if($(this).attr('class').indexOf('ui-selected') == -1)
      {
        clearSelectedCanvas();
      }
    })
    .on('contextmenu', function(e){
      e.preventDefault();
      $slideInfo.stop(false, true).hide('slide', {direction:'right'}, showHideTime, function(){
        $slideInfo.removeAttr('style').hide().css({height:$topology.css('height')});
        fadeEleIn = true;
      });
      toggleContextMenu(e);
    });
}

function toggleContextMenu(e)
{
  $slideInfo.stop(false, true).hide('slide', {direction:'right'}, showHideTime, function(){
    $slideInfo.removeAttr('style').hide().css({height:$topology.css('height')});
    fadeEleIn = true;
  });
  
  var evt = (e ? e : window.event);
  var source = evt.target || evt.srcElement;
  
  if(source.className.indexOf('canvasElement') == -1)
  {
    return;
  }
  
  $(source).off('mouseenter').css('border', '1px ridge red').addClass('ui-selected');
  
  selected.push(source);
  
  var left = $(source).css('left');
  var top = $(source).css('top');
  
  if($contextMenu.is(':hidden'))
  {
    var $delN = $('#deleteNodes');
    var $mergeN = $('#mergeNodes');
    var $splitN = $('#splitNodes');
    
    if(($('.ui-selected').size() > 1) && ($(source).attr('class').indexOf('ui-selected') != -1))
    {
      $delN.removeClass('single');
      $mergeN.on('click', function(e){
        var profiles = [];
        mergeSelected(source);
        $mergeN.off('click');
      }).show();
      $splitN.hide();
    }
    else if(($('.ui-selected').size() == 1) && ($(source).attr('class').indexOf('ui-selected') != -1))
    {
      $splitN.hide();
      if(parseInt($('#' + source.id + 'text').html()) > 1)
      {
        $splitN.on('click', function(e){
          splitSelected(source);
          $splitN.off('click');
        }).show();
        $delN.removeClass('single');
      }
      else
      {
        $delN.addClass('single');
      }
      $mergeN.hide();
    }
    
    $delN.on('click', function(e){
      deleteSelected();
      $delN.off('click');
    });
    
    var cmCss = {left: (parseInt(left) + (zoom.width / 2) + 160),
                 top: (parseInt(top) + (zoom.height / 2) + 75)};
    $contextMenu.css(cmCss).fadeIn(200);
    cmCss.left += (zoom.width / 2);
    $modalSplit.css(cmCss);
  }
}

function splitSelected(ele)
{
  $('#rangeDiv').empty();
  var text = theDoc.createElement('p');
  var slider = theDoc.createElement('input');
  slider.id = 'splitRange';
  slider.setAttribute('type', 'range');
  if(slider.type == 'range')
  {
    var max = (parseInt($('#' + ele.id + 'text').html()) - 1);
    var start = Math.floor(parseInt($('#' + ele.id + 'text').html()) / 2);
    $(text).html('Use the slider to split the nodes into two groups:');
    $(slider).attr('step', 1).attr('min', 1).attr('max', max).attr('value', start);
    var counter = theDoc.createElement('label');
    var delta = (parseInt($('#' + ele.id + 'text').html()) - Math.floor(parseInt($('#' + ele.id + 'text').html()) / 2));
    $(counter).attr('id', 'updateMe').html(start + ', ' + delta);
    var container = theDoc.createElement('div');
    var bottom = theDoc.createElement('label');
    $(bottom).attr('id', 'bottom');
    var top = theDoc.createElement('label');
    $(top).attr('id', 'top');
    $(bottom).html('1');
    $(top).html($('#' + ele.id + 'text').html());
    $(container).append(bottom, slider, top);

    function updateCounter(id)
    {
      $('#updateMe').html($('#splitRange').val() + ', ' + ($('#' + id + 'text').html() - $('#splitRange').val()));
    }
    
    $(slider).on('change', function(){updateCounter(ele.id)});
    
    $('#rangeDiv').append(text, counter, container);
    
    $('#splitAccept').on('click', function(){
      var y = parseInt($(ele).css('top'));
      var x = parseInt($(ele).css('left')) + parseInt(zoom.width);
      var splitEnd = parseInt($('#splitRange').val());
      var getSplitMacs = $(ele).data('mac').split(',');
      var splitData = {};
      var group1Macs = [];
      var group2Macs = [];
      var nodeObj = {};
      var newCount = parseInt($('#' + ele.id + 'text').html()) - splitEnd;
      nodeObj.pfile = $(ele).data('profile');
      
      splitData.src = ele;
      
      for(var i = 0; i < splitEnd; i++)
      {
        group1Macs.push(getSplitMacs[i]);
      }
      for(var i = splitEnd; i < getSplitMacs.length; i++)
      {
        group2Macs.push(getSplitMacs[i]);
      }
      
      splitData.srcNodeMacs = group1Macs.join();
      splitData.splNodeMacs = group2Macs.join();
      splitData.splCount = newCount.toString();
      splitData.srcCount = splitEnd.toString();
      nodeObj.count = splitData.splCount;
      
      addNodeToCanvas(nodeObj, 'split', x, y, undefined, undefined, splitData);
      
      $modalSplit.dialog('close');
      $('#splitAccept').off('click');
      $('.ui-selected').each(function(){
        $(this).removeClass('ui-selected').css('border', '');
      });
    });
    $modalSplit.dialog('open');
  }
  else
  {
    $(text).html('Use the input box to split the nodes into two groups:');
    var max = (parseInt($('#' + ele.id + 'text').html()) - 1);
    var start = Math.floor(parseInt($('#' + ele.id + 'text').html()) / 2);
    $(slider).attr('min', 1);
    $(slider).attr('max', max);
    $(slider).attr('value', start);
    var counter = theDoc.createElement('label');
    var delta = (parseInt($('#' + ele.id + 'text').html()) - Math.floor(parseInt($('#' + ele.id + 'text').html()) / 2));
    $(counter).attr('id', 'updateMe').html(start + ', ' + delta);
    var container = theDoc.createElement('div');
    $(container).append(bottom, slider, top);
    
    function updateCounter(id)
    {
      if(($('#splitRange').val() > $('#splitRange').attr('max')) || ($('#splitRange').val() < 1))
      {
        $('#splitRange').val('1');
      }
      $('#updateMe').html($('#splitRange').val() + ', ' + ($('#' + id + 'text').html() - $('#splitRange').val()));
    }
    
    $(slider).on('change', function(){updateCounter(ele.id)});
    
    $('#rangeDiv').append(text, counter, container);
    
    $('#splitAccept').on('click', function(){
      var y = parseInt($(ele).css('top'));
      var x = parseInt($(ele).css('left')) + parseInt(zoom.width);
      var splitData = {};
      var group1Macs = [];
      var group2Macs = [];
      var nodeObj = {};
      var splitEnd = parseInt($('#splitRange').val());
      var getSplitMacs = $(ele).data('mac').split(',');
      var newCount = parseInt($('#' + ele.id + 'text').html()) - splitEnd;
      nodeObj.pfile = $(ele).data('profile');
      
      splitData.src = ele;
      
      for(var i = 0; i < splitEnd; i++)
      {
        group1Macs.push(getSplitMacs[i]);
      }
      for(var i = splitEnd; i < getSplitMacs.length; i++)
      {
        group2Macs.push(getSplitMacs[i]);
      }
      
      splitData.srcNodeMacs = group1Macs.join();
      splitData.splNodeMacs = group2Macs.join();
      splitData.srcCount = newCount.toString();
      splitData.splCount = splitEnd.toString();
      nodeObj.count = splitData.splCount;
      
      addNodeToCanvas(nodeObj, 'split', x, y, undefined, undefined, splitData);
      
      $modalSplit.dialog('close');
      $('#splitAccept').off('click');
    });
    $modalSplit.dialog('open');
  }
  $contextMenu.fadeOut(100);
}

function clearTopoListeners(topo)
{
  topo.off('mouseup')
      .off('mousedown')
      .off('mousemove')
      .off('mouseout')
      .selectable('destroy')
      .droppable('destroy');
}

function appendTopoListeners(topo)
{
  var droppableOptions = {accept:'.ui-draggable'};
  topo
    .selectable({appendTo:'#' + topo.attr('id'),
                 distance:30,
                 cancel:'.canvasElement',
                 filter:'.canvasElement'})
    .on('selectablestart', function(e, ui){
      e.preventDefault();
      $('.canvasElement').off('mouseenter');
    })
    .on('click', function(){
      $profileInfo.fadeOut(200);
      clearProfileSelected();
      clicked = false;
    })
    .on('selectableselecting', function(e, ui){
      selected.push(ui.selecting);
      $(ui.selecting).css('border', '1px ridge red');
    })
    .on('selectableunselecting', function(e, ui){
      for(var i in selected)
      {
        if(selected[i] == ui.unselecting)
        {
          delete selected[i];
        }
      }
      $(ui.unselecting).css('border', '');
    })
    .on('selectablestop', function(e, ui){
      $('.canvasElement').on('mouseenter', function(e){
        e.stopImmediatePropagation();
        handleCanvasEleHover(e);
      });
    })
    .mousedown(function(e){
      var evt = (e ? e : window.event);
      var source = evt.target || evt.srcElement;
      if(e.which == 2)
      {
        topoDrag = true;
        $topology.css('cursor', 'move');
        oldX = e.clientX;
        oldY = e.clientY;
        $topology.off('mouseenter');
      }
      else if((e.which == 1) && !$slideInfo.is(':hidden') 
      && ($(source).attr('class') != undefined) 
      && ($(source).attr('class').indexOf('canvasElement') == -1))
      {
        $slideInfo.stop(false, true).hide('slide', {direction:'right'}, showHideTime, function(){
          $slideInfo.removeAttr('style').hide().css({height:$topology.css('height')});
          fadeEleIn = true;
        });
        $('.ui-selected').each(function(){
          $(this).removeClass('ui-selected').css('border', '');
        });
      }
      return false;
    })
    .mouseup(function(e){
      if(e.which == 2)
      {
        topoDrag = false;
        $topology.css('cursor', '');
        $topology.on('mouseenter', function(){
          e.stopImmediatePropagation();
          handleCanvasEleHover(e);
        });
        $('.canvasElement').each(function(){
          nodeTopology[$(this).attr('id')].coordinates.x = parseInt($(this).css('left'));
          nodeTopology[$(this).attr('id')].coordinates.y = parseInt($(this).css('top'));
        });
      }
    })
    .mouseout(function(e){
      if(e.which == 2)
      {
        topoDrag = false;
        $topology.css('cursor', '');
        $topology.on('mouseenter', function(){
          e.stopImmediatePropagation();
          handleCanvasEleHover(e);
        });
      }
    })
    .mousemove(function(e){
      if(topoDrag)
      {
        $('.canvasElement').each(function(){
          var newPos = {left:parseInt($(this).css('left')) + parseInt(e.clientX - oldX),
                        top:parseInt($(this).css('top')) + parseInt(e.clientY - oldY)};
          $(this).css(newPos);
        });
        // shift background while dragging
        var backgroundShiftLeft = parseInt(e.clientX - oldX) + parseInt($topology.css('background-position').split(' ')[0]);
        var backgroundShiftTop = parseInt(e.clientY - oldY) + parseInt($topology.css('background-position').split(' ')[1]);
        var backgroundShift = backgroundShiftLeft + 'px ' + backgroundShiftTop + 'px';
        $topology.css('background-position', backgroundShift);
        
        handleOffscreenIndicators();
        oldX = e.clientX;
        oldY = e.clientY;
      }
    })
    .droppable(droppableOptions)
    .on('dropover', function(event, ui){
      $topology.addClass('over');
    })
    .on('dropout', function(event, ui){
      $topology.removeClass('over');
    })
    .on('drop', function(event, ui){
      noclick = true;
      $topology.removeClass('over');
      $dragMe.draggable('disable');
      placeBackgroundImage($dragMe);
      var dragMeCss = {border:'2px dashed black',
                       opacity:'0.4'};
      $dragMe.css(dragMeCss);
      
      if(ui.draggable.attr('class').indexOf('canvasElement') == -1 
      && ui.draggable.attr('class').indexOf('ui-dialog') == -1)
      {
        var nodeObj = {};
        nodeObj.pfile = selectedProfile;
        nodeObj.count = $('#nodeNumber').val();
        
        clearProfileSelected();
        
        var x = ui.helper.offset().left - $topology.offset().left;
        var y = ui.helper.offset().top - $topology.offset().top;
        addNodeToCanvas(nodeObj, 'drop', x, y);
      }
      else if(ui.draggable.attr('class').indexOf('canvasElement') != -1 
      && ui.draggable.attr('class').indexOf('ui-dialog') == -1)
      {
        var x = ui.helper.offset().left - $topology.offset().left;
        var y = ui.helper.offset().top - $topology.offset().top;
        var index = ui.draggable.attr('id');
        nodeTopology[index].coordinates = {x:x, y:y};
      }
    });
    
  var draggableOptions = {helper:'clone',
                  tolerance: 'pointer',
                  cursorAt: {top:25,left:15},
                  cursor: 'move',
                  containment: '#' + topo.attr('id'),
                  revert: 'invalid'};
  
  $dragMe.draggable(draggableOptions)
    .on('dragstart', function(event, ui){
      var evt = (event ? event : window.event);
      var source = evt.target || evt.srcElement;
      source.style.opacity = '0.4';
    })
    .on('dragstop', function(event, ui){
      var evt = (event ? event : window.event);
      var source = evt.target || evt.srcElement;
      var dragMeCss = {border:'2px dashed black',
                       opacity:'0.4'};
      $dragMe.css(dragMeCss);
    })
    .draggable('disable');
    
  var dragMeCss = {border:'2px dashed black',
                   opacity:'0.4'};
  $dragMe.css(dragMeCss);
}

function mergeSelected(base)
{
  $contextMenu.fadeOut(100);
  var div = theDoc.createElement('div');
  var baseProfile = $(base).data('profile');
  var baseMac = $(base).data('mac');
  var baseVendor = '';
  var basePortset = '';
  var baseIp = '';
  var baseInterface = '';
  
  for(var i in nodeTopology)
  {
    if(baseMac == nodeTopology[i].mac)
    {
      baseVendor = nodeTopology[i].vendor;
      basePortset = nodeTopology[i].portset;
      baseIp = nodeTopology[i].ip;
      baseInterface = nodeTopology[i].iface;
      break;
    }
  }
  
  var keepGoing = true;
  var id = '';
  var $list = $('.ui-selected');
  $list.each(function(){
    id = $(this).attr('id');
    if(!keepGoing)
    {
      return;
    }
    if(nodeTopology[id].pfile != baseProfile || nodeTopology[id].vendor != baseVendor
       || nodeTopology[id].portset != basePortset || nodeTopology[id].ip != baseIp
       || nodeTopology[id].iface != baseInterface)
    {
      alert('In order to merge nodes, all nodes to merge must have the same general characteristics');
      keepGoing = false;
      return;
    }
  });
  if(keepGoing)
  {
    $list.each(function(){
      id = $(this).attr('id');
      if(id != base.id)
      {
        nodeTopology[base.id].mac += ',' + nodeTopology[id].mac;
        nodeTopology[base.id].count = parseInt(nodeTopology[base.id].count) + parseInt(nodeTopology[id].count);
        delete nodeTopology[id];
        $(this).fadeOut(200, function(){$(this).remove();});
      }
    });
    $('#' + base.id + 'text').html(nodeTopology[base.id].count);
  }
  else
  {
    return;
  }
}

function updateCount()
{
  var $change = $('#' + hoverNode + 'text');
  var oldValue = $change.html();
  var newValue = $('#changeCount').val();
  if(parseInt(newValue) < 0)
  {
    alert('Cannot change a node amount to a negative value!');
    $('#' + hoverNode).removeClass('ui-selected');
    $slideInfo.stop(false, true).hide('slide', {direction:'right'}, showHideTime, function(){
      $slideInfo.removeAttr('style').hide().css({height:$topology.css('height')});
      fadeEleIn = true;
    });
    return;
  }
  var delta = newValue - oldValue;
  if(delta > 0)
  {
    nodeTopology[hoverNode].count = newValue;
    $change.html(newValue);
    var ip = [];
    var ipType= '';
    if(nodeTopology[hoverNode].ip == 'DHCP')
    {
      ip = [];
      ipType = 'DHCP';
    }
    else
    {
      ip = nodeTopology[hoverNode].ip;
      ipType = '';
    }
    var profile = nodeTopology[hoverNode].pfile;
    var portset = nodeTopology[hoverNode].portset;
    var vendor = nodeTopology[hoverNode].vendor;
    var ethInterface = nodeTopology[hoverNode].iface;
    var count = delta;
    now.createHoneydNodes(ipType, ip.ip1, ip.ip2, ip.ip3, ip.ip4, profile, portset, vendor, ethInterface, count);
    nodes = [];
    var setMAC = [];
    
    if(typeof now.GetNodes == 'function')
    {
      now.GetNodes(function(nodesList){
        nodes = nodesList;
        var add = true;
        for(var i in nodes)
        {
          add = true;
          for(var j in haveMac)
          {
            if(haveMac[j] == nodes[i].mac)
            {
              add = false;
              break;
            }
          }
          if(add)
          {
            haveMac.push(nodes[i].mac);
            setMAC.push(nodes[i].mac);
          } 
        }
        nodeTopology[hoverNode].mac += ',' + setMAC.join();
        $('#' + hoverNode).data('mac', nodeTopology[hoverNode].mac);
      });
    }
    else
    {
      alert('Could not update node count: No connection to server');
    }
  }
  else if(delta < 0)
  {
    var macsToRemove = [];
    if(newValue <= 0)
    {
      macsToRemove = nodeTopology[hoverNode].mac.split(',');
      now.deleteNodes(macsToRemove);
      delete nodeTopology[hoverNode];
      $('#' + hoverNode).remove();
    }
    else
    {
      nodeTopology[hoverNode].count = newValue;
      $change.html(newValue);
      macsToRemove = [];
      var macsInNode = nodeTopology[hoverNode].mac.split(',').reverse();
      var newMacAssignment = [];
      delta = Math.abs(delta);
      for(var i = 0; i < delta; i++)
      {
        macsToRemove.push(macsInNode[i]);
        delete macsInNode[i];
      }
      
      macsInNode = macsInNode.reverse();
      for(var i in macsInNode)
      {
        if(macsInNode[i] != undefined)
        {
          newMacAssignment.push(macsInNode[i]);
        }
      }
      nodeTopology[hoverNode].mac = newMacAssignment.join(',');
      
      now.deleteNodes(macsToRemove);
    }
  }
  hoverhold = false;
  $('#' + hoverNode).removeClass('ui-selected').css('border', '');
  $slideInfo.stop(false, true).hide('slide', {direction:'right'}, showHideTime, function(){
    $slideInfo.removeAttr('style').hide().css({height:$topology.css('height')});
    fadeEleIn = true;
  });
}

function handleHelpPaneSlide()
{
  if($('#helpPane').is(':hidden'))
  {
    $('#helpPane').slideDown(400);
  }
  else
  {
    $('#helpPane').slideUp(400);
  }
}

function deleteSelected()
{
  var deleteNodes = [];
  $('.ui-selected').each(function(){
    if(nodeTopology[$(this).attr('id')] != undefined)
    {
      delete nodeTopology[$(this).attr('id')];
    }
    var splitMac = $(this).data('mac').split(',');
    for(var i in splitMac)
    {
      deleteNodes.push(splitMac[i]);
    }
    $(this).remove();
  });
  
  $contextMenu.fadeOut(100);
  $slideInfo.stop(false, true).hide('slide', {direction:'right'}, showHideTime, function(){
    $slideInfo.removeAttr('style').hide().css({height:$topology.css('height')});
    fadeEleIn = true;
  });
  
  now.deleteNodes(deleteNodes);
}

function clearSelectedCanvas()
{
  for(var i in selected)
  {
    $(selected[i]).removeClass('ui-selected').css('border', '');
  }
  $('.canvasElement').on('mouseenter', function(e){
    e.stopImmediatePropagation();
    handleCanvasEleHover(e);
  });
  $contextMenu.fadeOut(100);
  $slideInfo.stop(false, true).hide('slide', {direction:'right'}, showHideTime, function(){
    $slideInfo.removeAttr('style').hide().css({height:$topology.css('height')});
    fadeEleIn = true;
  });
  fadeEleIn = true;
  hoverhold = false;
  selected = [];
}

function generateTempCanvas()
{
  var tabDiv = theDoc.createElement('div');
  tabDiv.id = 'tempTab';
  tabDiv.setAttribute('onclick', 'clearSelectedCanvas()');
  var canvasDiv = theDoc.createElement('div');
  canvasDiv.id = 'nodeCanvasTemp';
  canvasDiv.className = 'canvas sameHeight';
  tabDiv.appendChild(canvasDiv);
  
  $topology = $(canvasDiv);
  
  var draggableOptions = {helper:'clone',
                  tolerance: 'pointer',
                  cursorAt: {top:25,left:15},
                  cursor: 'move',
                  containment: '#' + $topology.attr('id'),
                  revert: 'invalid'};
  
  $dragMe.draggable(draggableOptions)
    .on('dragstart', function(event, ui){
      var evt = (event ? event : window.event);
      var source = evt.target || evt.srcElement;
      source.style.opacity = '0.4';
    })
    .on('dragstop', function(event, ui){
      var evt = (event ? event : window.event);
      var source = evt.target || evt.srcElement;
      var dragMeCss = {border:'2px dashed black',
                       opacity:'0.4'};
      $dragMe.css(dragMeCss);
    })
    .draggable('disable');
    
  var dragMeCss = {border:'2px dashed black',
                   opacity:'0.4'};
  $dragMe.css(dragMeCss);
  
  var droppableOptions = {accept:'.ui-draggable'};
  // set up custom droppable here
  $topology.droppable(droppableOptions)
  .on('dropover', function(event, ui){
    $topology.addClass('over');
  })
  .on('dropout', function(event, ui){
    $topology.removeClass('over');
  })
  .on('drop', function(event, ui){
    noclick = true;
    $topology.removeClass('over');
    $dragMe.draggable('disable');
    placeBackgroundImage($dragMe);
    var dragMeCss = {border:'2px dashed black',
                     opacity:'0.4'};
    $dragMe.css(dragMeCss);
    
    if((ui.draggable.attr('class').indexOf('canvasElement') == -1) 
    && (ui.draggable.attr('class').indexOf('ui-dialog') == -1))
    {
      var nodeObj = {};
      nodeObj.pfile = selectedProfile;
      nodeObj.count = $('#nodeNumber').val();
      
      clearProfileSelected();
      
      var x = ui.helper.offset().left - $topology.offset().left;
      var y = ui.helper.offset().top - $topology.offset().top;
      $topology.remove();
      $topology = '';
      
      var ipType = '';
      var ip1, ip2, ip3, ip4 = '0';
      if(nodeTopology[eleCount].ip != 'DHCP')
      {
        ip1 = nodeTopology[eleCount].ip.ip1;
        ip2 = nodeTopology[eleCount].ip.ip2;
        ip3 = nodeTopology[eleCount].ip.ip3;
        ip4 = nodeTopology[eleCount].ip.ip4;
      }
      else
      {
        ipType = 'DHCP';
      }
      var profile = nodeTopology[eleCount].pfile;
      var portset = nodeTopology[eleCount].portset.toString();
      var vendor = nodeTopology[eleCount].vendor;
      var ethInterface = nodeTopology[eleCount].iface;
      var count = nodeTopology[eleCount].count.toString();
      delete nodeTopology[eleCount];
      if((typeof now.createHoneydNodes == 'function') && (typeof now.GetNodes == 'function'))
      {
        now.createHoneydNodes(ipType, ip1, ip2, ip3, ip4, profile, portset, vendor, ethInterface, count);
  
        nodes = [];
        var setMAC = [];
  
        now.GetNodes(function(nodesList){
          nodes = nodesList;
          nodeList = nodesList;
          var add = true;
          for(var i in nodes)
          {
            add = true;
            for(var j in haveMac)
            {
              if(haveMac[j] == nodes[i].mac)
              {
                add = false;
                break;
              }
            }
            if(add)
            {
              haveMac.push(nodes[i].mac);
              setMAC.push(nodes[i].mac);
            } 
          }
        });
      }
      else
      {
        alert('Could not create honeyd nodes: No connection to server');
      }
      
      prepopulateCanvasWithNodes(function(){
        if($topology != undefined && $topology != '')
        {
          appendTopoListeners($topology);
          handleOffscreenIndicators(); 
        }
        else
        {
          generateTempCanvas();
        }
        adjustColumns();
      });
    }
  });
  $topoHook.append(tabDiv);
}

$(function(){
  $dragMe = $('#dragMe');
  $slideInfo = $('#slideInfo');
  $slideInfo.hide();
  $profileInfo = $('#profileFadeInfo');
  $profileInfo.hide();
  $contextMenu = $('#nodeContextMenu');
  $contextMenu.hide();
  $topoHook = $('#canvasContainer');
  $topology = '';
  $modalSplit = $('#modalDialog');
  $modalSplit.dialog({
    appendTo: '#container',
    height: 180,
    width: 200,
    draggable: true,
    autoOpen: false,
    closeOnEscape: true,
    modal: true,
    show: 200,
    hide: 200
  })
  
  var fix = $('#pageWrap')[0];
  fix = fix.parentNode;
  fix = $(fix);
  fix.css('height', '800px');
     
  $(document).keydown(function(e){
    if(e.keyCode == 46)
    {
      deleteSelected();
    }
  });      
            
  now.ready(function(){
    setUpSelects();
    updateDragHeader();
          
    $('#slideInfo').hide();
    $('#saveMessage').hide();
    $('#placeHolder').hide();
          
    now.GetNodes(function(nlist){
      for(var i in nlist)
      {
        macsTotal.push(nlist[i].mac);
      }
      repopulateNodeCanvas(function(){
        prepopulateCanvasWithNodes(function(){
          if($topology != undefined && $topology != '')
          {
            appendTopoListeners($topology);
            handleOffscreenIndicators(); 
          }
          else
          {
            // add TempCanvas to $topoHook; TempCanvas will require different droppable rules
            generateTempCanvas();
          }
          adjustColumns();
          $('#pageWrap').attr('style', '');
        });
      });
    });
  });
  setInterval(function(){
    if(typeof now.WriteWysiwygTopology == 'function')
    {
      now.WriteWysiwygTopology(nodeTopology, function(){
        console.log('WriteWysiwygTopology finished');
        $('#saveMessage').stop(false, true).show().delay(1000).fadeOut('slow');
      });
    }
    else
    {
      console.log('Could not write topology out to file: No connection to server!');
    }
  }, (1000 * 60 * 5));
});

$(window).resize(function(){
  handleOffscreenIndicators();
  var $sameHeightDivs = $('.sameHeight');
  $sameHeightDivs.css({height:'auto'});
  adjustColumns();
});

$(window).keydown(function(event){
  if(!(event.shiftKey && event.ctrlKey && (event.keyCode == 83 || event.keyCode == 72)) && !(event.which == 19))
  {
    event.stopPropagation();
  }
  else if(event.keyCode == 83)
  {
    if(typeof now.WriteWysiwygTopology == 'function')
    {
      now.WriteWysiwygTopology(nodeTopology, function(){
        console.log('WriteWysiwygTopology finished');
        $('#saveMessage').stop(false, true).show().delay(1000).fadeOut('slow');
      });
    }
    else
    {
      console.log('Could not write topology out to file: No connection to server!');
    }
    event.stopPropagation();
  }
  else if(event.keyCode == 72)
  {
    handleHelpPaneSlide();
  }
});

window.addEventListener('beforeunload', function(){
  if(typeof now.WriteWysiwygTopology == 'function')
  {
    now.WriteWysiwygTopology(nodeTopology, function(){
      console.log('onUnload WriteWysiwygTopology finished');
      $('#saveMessage').stop(false, true).show().delay(1000).fadeOut('slow');    
    });
  }
  else
  {
    return 'No connection to server, cannot save topology to file.';
  }
});