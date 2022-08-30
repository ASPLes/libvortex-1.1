#!/usr/bin/lua
--  Lua Vortex:  Lua bindings for Vortex Library
--  Copyright (C) 2022 Advanced Software Production Line, S.L.
--
--  This program is free software; you can redistribute it and/or
--  modify it under the terms of the GNU Lesser General Public License
--  as published by the Free Software Foundation; either version 2.1
--  of the License, or (at your option) any later version.
--
--  This program is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
--  Lesser General Public License for more details.
--
--  You should have received a copy of the GNU Lesser General Public
--  License along with this program; if not, write to the Free
--  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
--  02111-1307 USA
--  
--  You may find a copy of the license under this software is released
--  at COPYING file. This is LGPL software: you are welcome to develop
--  proprietary applications using this library without any royalty or
--  fee but returning back any change, improvement or addition in the
--  form of source code, project image, documentation patches, etc.
--
--  For commercial support on build BEEP enabled solutions contact us:
--          
--      Postal address:
--         Advanced Software Production Line, S.L.
--         C/ Antonio Suarez Nº 10, 
--         Edificio Alius A, Despacho 102
--         Alcalá de Henares 28802 (Madrid)
--         Spain
--
--      Email address:
--         info@aspl.es - http://www.aspl.es/vortex


require "vortex"
require "vortex.sasl"
require "vortex.tls"
require "vortex.alive"
require "os"
require "io"

-- regression test uris
regression_uri      = "http://iana.org/beep/transient/vortex-regression"
regression_uri_zero = "http://iana.org/beep/transient/vortex-regression/zero"
regression_uri_deny           = "http://iana.org/beep/transient/vortex-regression/deny"
regression_uri_deny_supported = "http://iana.org/beep/transient/vortex-regression/deny_supported"

--- define a printf
printf = function(s,...)
	    return io.write(s:format(...))
         end -- function

function error (message) 
   print ("ERROR: " .. message)
end

function info (message) 
   print ("   " .. message)
end

function test_01 ()
   -- create context 
   ctx = vortex.ctx.new ()

   -- access to reference counting
   print ("   Test 01: Ref count received: " .. tostring (ctx.ref_count))
   if ctx.ref_count ~= 1 then
      print ("ERROR: expected to find ref count equal to 1 but found: " .. tostring (ctx.ref_count))
      return false
   end

   -- call to terminate ctx
   vortex.ctx.unref (ctx)
   vortex.ctx.unref (ctx)

   return true
end

function test_02 ()
   -- create context 
   ctx = vortex.ctx.new ()

   -- access to reference counting
   print ("   Ref count received: " .. tostring (ctx.ref_count))
   if ctx.ref_count ~= 1 then
      print ("ERROR: expected to find ref count equal to 1 but found: " .. tostring (ctx.ref_count))
      return false
   end

   return true
end

function test_03 ()
   -- create context 
   ctx = vortex.ctx.new ()

   -- init context
   if not vortex.ctx.check (ctx) then
      print ("ERROR: expected to find positive reply for vortex.ctx check, but found failure")
      return false
   end

   if vortex.ctx.check ({}) then
      print ("ERROR: expected to NOT find positive reply for vortex.ctx check")
      return false
   end

   -- access to reference counting
   if ctx.ref_count ~= 1 then
      print ("ERROR: expected to find ref count equal to 1 but found: " .. tostring (ctx.ref_count))
      return false
   end

   return true
end

function test_03 ()
   -- create context 
   ctx = vortex.ctx.new ()

   -- init context
   if not vortex.ctx.check (ctx) then
      print ("ERROR: expected to find positive reply for vortex.ctx check, but found failure")
      return false
   end

   -- access to reference counting
   if ctx.ref_count ~= 1 then
      print ("ERROR: expected to find ref count equal to 1 but found: " .. tostring (ctx.ref_count))
      return false
   end

   return true
end

function test_04 ()
   -- create context 
   ctx = vortex.ctx.new ()

   -- init context
   if not vortex.ctx.init (ctx) then
      print ("ERROR: expected to find proper vortex.ctx initialization..")
      return false
   end

   print ("   Test 04: checking ctx created..")
   if not vortex.ctx.check (ctx) then
      print ("ERROR: expected to find positive reply for vortex.ctx check, but found failure")
      return false
   end

   -- connect to localhost server
   print ("   Test 04: calling to create connection..")
   conn = vortex.connection.new (ctx, "localhost", "44010")
   
   -- check result
   if not vortex.connection.is_ok (conn) then
      print ("ERROR: expected to find proper connection error but found failure..")
      return false
   end

   -- now check some data
   if conn.host ~= "localhost" then
      print ("ERROR: expected to find connection host value 'localhost' but found: " .. conn.host)
      return false
   end

   if conn.port ~= "44010" then
      print ("ERROR: expected to find connection port value '44010' but found: " .. conn.port)
      return false
   end

   -- check connection reference 
   if not vortex.connection.check (conn) then
      print ("ERROR: expected to find proper connection.check but found failure..")
      return false
   end

   -- check connection reference 
   print ("   Test 04: calling to check vortex.connection.check with an empty table..(should fail)")

   return true
end

function test_04_a ()
    -- call to initialize a context 
    ctx = vortex.ctx.new ()
    
    -- call to init ctx 
    if not ctx:init () then
        error ("Failed to init Vortex context")
        return false
    end

    -- call to create a connection
    conn = vortex.connection.new (ctx, "localhost", "44010")

    -- check connection status after if 
    if not conn:is_ok () then
        error ("Expected to find proper connection result, but found error. Error code was: " .. tostring(conn.status) .. ", message: " .. conn.error_msg)
        return false
    end

    -- set some data
    conn:set_data ('value', 1)
    conn:set_data ('value2', 2)
    conn:set_data ('boolean', true)

    -- now set a connection to also check it is released
    conn2 = vortex.connection.new (ctx, "localhost", "44010")

    -- check connection status after if 
    if not conn2:is_ok () then
        error ("Expected to find proper connection result, but found error. Error code was: " .. tostring(conn.status) + ", message: " .. conn.error_msg)
        return false
    end

    conn:set_data ('conn', conn2)

    -- recover data
    if conn:get_data ('value') ~= 1 then
        error ("Expected to find value == 1 but found: " .. tostring (conn.get_data ('value')))
        return false
    end
    if conn:get_data ('value2') ~= 2 then
        error ("Expected to find value2 == 2 but found: " .. tostring (conn.get_data ('value2')))
        return false
    end
    if not conn:get_data ('boolean') then
        error ("Expected to find boolean == True but found: " .. tostring (conn.get_data ('boolean')))
        return false
    end

    conn3 = conn:get_data ('conn')

    -- check conn references
    if conn2.id ~= conn3.id then
        error ("Expected to find same connection references but found they differs: " .. tostring (conn2.id) .. " != " .. tostring (conn3.id))
	return false
    end

    return true
end

function test_05 ()
   -- create context 
   ctx = vortex.ctx.new ()

   -- init context
   if not vortex.ctx.init (ctx) then
      print ("ERROR: expected to find proper vortex.ctx initialization..")
      return false
   end

   -- connect to localhost server
   print ("   Test 05: calling to create connection..")
   conn = vortex.connection.new (ctx, "localhost", "44010")
   
   -- check result
   if not vortex.connection.is_ok (conn) then
      print ("ERROR: expected to find proper connection error but found failure..")
      return false
   end

   -- call to close
   if not vortex.connection.close (conn) then
      print ("ERROR: expected to find proper connection close but found failure")
   end

   return true
end

function test_06 () 

   queue = vortex.asyncqueue.new ()

   -- push some data
   vortex.asyncqueue.push (queue, 10)
   vortex.asyncqueue.push (queue, "hola")
   vortex.asyncqueue.push (queue, {1, 2, 3})

   -- check number of items
   if vortex.asyncqueue.items (queue) ~= 3 then
      print ("ERROR: expected to find 3 items but found ".. tostring (vortex.asyncqueue.items (queue)))
      return false
   end

   -- pop items
   print ("Test 06: Checking number..")
   number = vortex.asyncqueue.pop (queue)
   if type(number) ~= "number" then
      print ("ERROR: expected to find number object but found ".. type (number))
      return false
   end
   -- check number value
   if number ~= 10 then
      print ("ERROR: expected to find 10 but found " .. tostring (number))
      return false
   end

   -- check string
   print ("Test 06: Checking strings..")
   value = vortex.asyncqueue.pop (queue)
   if type(value) ~= "string" then
      print ("ERROR: expected to find number object but found ".. type (value))
      return false
   end
   -- check number value
   if value ~= "hola" then
      print ("ERROR: expected to find 10 but found " .. tostring (value))
      return false
   end

   -- check table
   print ("Test 06: Checking tables..")
   table = vortex.asyncqueue.pop (queue)
   if type(table) ~= "table" then
      print ("ERROR: expected to find number object but found something different..")
      return false
   end

   if table[1] ~= 1 or table[2] ~= 2 or table[3] ~= 3 then
      print ("ERROR: expected to find {1,2,3} table but found something different..")
      return false
   end

   -- checking memory is released even if no pop operation happens
   iterator = 0
   while (iterator < 10) do
      queue = vortex.asyncqueue.new ()
      vortex.asyncqueue.push (queue, {1, 2, 3})
      vortex.asyncqueue.push (queue, {4, 5, 6})

      iterator = iterator + 1
   end

   return true
end

function test_07_on_close (conn, lista)
   print ("Test 07: Received call on test 07: test_07_on_close")

   print ("  conn value is: " .. tostring(conn))
   print ("  lista value is: " .. tostring(lista))

   -- get the queue 
   queue = lista[1]

   -- push a value
   vortex.asyncqueue.push (queue, 2)
end

function test_07 ()
   -- create context 
   ctx = vortex.ctx.new ()

   -- init context
   if not vortex.ctx.init (ctx) then
      print ("ERROR: expected to find proper vortex.ctx initialization..")
      return false
   end

   -- connect to localhost server
   print ("   Test 07: calling to create connection..")
   conn = vortex.connection.new (ctx, "localhost", "44010")
   
   -- check result
   if not vortex.connection.is_ok (conn) then
      print ("ERROR: expected to find proper connection error but found failure..")
      return false
   end

   -- create the queue
   queue = vortex.asyncqueue.new ()

   -- call to configure connection close
   print ("Test 07: checking set on close..")
   vortex.connection.set_on_close (conn, test_07_on_close, {queue, 10, 22})

   print ("Test 07: closing connection..")
   vortex.connection.shutdown (conn)

   -- block until we receive the reply
   print ("   ..closed, now wait for result..")
   result = vortex.asyncqueue.pop (queue)

   if result ~= 2 then
      print ("ERROR: expected to receive 2 value from wait but found " .. 2)
      return false
   end

   return true
end

function queue_reply (conn, channel, frame, data)

   print ("Test 08: frame received, connection is: " .. tostring (conn))
   if not conn:is_ok () then
      error ("Test 08: Found connection that is not ok inside frame received")
      os.exit (-1)
   end

   if not channel:check () then
      error ("Test 08: Found channel wrong reference inside frame received")
      os.exit (-1)
   end

   -- push frame into queue
   data:push (frame)
end

function queue_reply_unchecked (conn, channel, frame, data)
   printf ("Test 12: called queue reply unchecked..")
   -- push frame into queue
   data:push (frame)
end

function test_08 ()
   -- create context 
   ctx = vortex.ctx.new ()
   
   -- init context
   if not vortex.ctx.init (ctx) then
      print ("ERROR: expected to find proper vortex.ctx initialization..")
      return false
   end
   
   -- connect to localhost server
   print ("   Test 08: calling to create connection..")
   conn = vortex.connection.new (ctx, "localhost", "44010")
   
   -- check result
   if not vortex.connection.is_ok (conn) then
      print ("ERROR: expected to find proper connection error but found failure..")
      return false
   end

   -- create the queue
   queue = vortex.asyncqueue.new ()

   -- open a channel
   channel = conn:open_channel (0, regression_uri, queue_reply, queue)

   -- check channel result
   if channel == nil then
      print ("ERROR: expected proper channel creation but nil reference was found..")
      return false
   end

   print ("Test 08: channel created " .. tostring (channel.number) .. ", running profile: " .. channel.profile .. "!, sending content")
   
   -- send a message 
   if channel:send_msg ("Esto es una prueba...", 21) == nil then
      print ("ERROR: expected to find proper send operation..")
      return false
   end

   -- get replies
   print ("Test 08: getting reply from queue..")
   frame = queue:pop ()

   print ("Frame received: " .. tostring (frame))

   if frame == nil then
      print ("ERROR: expected to find proper frame result..")
      return false
   end
   
   if frame.payload ~= "Esto es una prueba..." then
      print ("ERROR: expected to find frame content: 'Esto es una prueba...' but found: '" .. frame.payload .. "'")
      return false
   end

   -- check frame type 
   if frame.type ~= "RPY" then
      print ("ERROR: expected to frame frame type RPY but found: " .. frame.type)
      return false
   end
   
   return true
end

function test_09 ()
   -- create context 
   ctx = vortex.ctx.new ()
   
   -- init context
   if not vortex.ctx.init (ctx) then
      print ("ERROR: expected to find proper vortex.ctx initialization..")
      return false
   end
   
   -- connect to localhost server
   print ("   Test 09: calling to create connection..")
   conn = vortex.connection.new (ctx, "localhost", "44010")
   
   -- check result
   if not vortex.connection.is_ok (conn) then
      print ("ERROR: expected to find proper connection error but found failure..")
      return false
   end

   -- create the queue
   queue = vortex.asyncqueue.new ()

   -- open a channel
   channel = conn:open_channel (0, regression_uri_zero, queue_reply, queue)

   -- check channel result
   if channel == nil then
      print ("ERROR: expected proper channel creation but nil reference was found..")
      return false
   end

   -- send a message 
   if channel:send_msg ("\0\0\0\0", 4) == nil then
      print ("ERROR: expected to find proper send operation..")
      return false
   end

   -- get replies
   print ("Test 09: getting zero reply from queue..")
   frame = queue:pop ()

   print ("Frame received: " .. tostring (frame))

   if frame == nil then
      print ("ERROR: expected to find proper frame result..")
      return false
   end
   
   if frame.payload ~= "\0\0\0\0" then
      print ("ERROR: expected to find frame content: '\0\0\0\0' but found: '" .. frame.payload .. "'")
      return false
   end

   return true
end

function test_09_a_on_channel (number, channel, conn,  queue)

    info ("Received async channel notification, number: " .. tostring (number) )
    queue:push (channel)
    return
end

function test_09_a ()

   -- create a context
   ctx = vortex.ctx.new ()

   -- call to init ctx 
   if not ctx:init () then
      error ("Failed to init Vortex context")
      return false
   end

   -- create a queue
   queue = vortex.asyncqueue.new ()

   -- create connection
   conn = vortex.connection.new (ctx, "localhost", "44010")

   -- check connection status
   if not conn:is_ok () then
      error ("Expected to find connection status ok, but found a failure: " .. conn.status_msg)
      return false
   end

   -- ok now create channel without waiting
   info ("Creating channel with async notification..")
   conn:open_channel (0, regression_uri, nil, nil, test_09_a_on_channel, queue)

   -- wait for response
   channel = queue:pop ()

   -- check channel value here and send some content
   info ("Channel received in main thread: " .. tostring (channel.number))

   -- send the message
   message = "This is a test message after async channel notification"
   iterator = 0
   channel:set_frame_received (function (conn, channel, frame, queue)
				  queue:push (frame) 
			       end, 
			       queue)
   
   
   while iterator < 10 do
      -- send message 
      channel:send_msg (message, #message)

      -- now get a frame
      frame = queue:pop ()
      if not frame then
	 error ("Expected to find frame reply but found None reference")
	 return false
      end
      
      if frame.payload ~= message then
	 error ("Expected to receive different message but found: " .. frame.payload .. ", rather: " .. message)
	 return false
      end

      -- next position
      iterator = iterator + 1
   end

   -- NOTE: the test do not close conn or channe (this is intentional)
   return true
end

function test_09_b ()

   -- create a context
   ctx = vortex.ctx.new ()

   -- call to init ctx 
   if not ctx:init () then
      error ("Failed to init Vortex context")
      return false
   end

   queue = vortex.asyncqueue.new ()

   -- create connection
   conn = vortex.connection.new (ctx, "localhost", "44010")

   -- check connection status
   if not conn:is_ok () then
      error ("Expected to find connection status ok, but found a failure: " .. conn.status_msg)
      return false
   end

   -- ok now create channel without waiting
   conn:open_channel (0, regression_uri_deny_supported, nil, nil, test_09_a_on_channel, queue)

   -- wait for response
   channel = queue:pop ()

   -- check channel value here and send some content
   if channel then
      error ("Expected to find None value at channel reference..")
      return false
   end

   -- ok check again connection and create a channel
   if not conn:is_ok () then
      error ("Expected to find connection properly created..")
      return false
   end

   channel = conn:open_channel (0, regression_uri)
   if not channel then
      error ("Expected to find proper channel..")
      return false
   end

   -- send some data
   iterator = 0
   channel:set_frame_received (function (conn, channel, frame, queue)
				  queue:push (frame)
			       end, queue)
   message = "This is a test at channel error expected.."
   while iterator < 10 do
      channel:send_msg (message, #message)

      -- now get a frame
      frame = queue:pop ()
      if not frame then
	 error ("Expected to find frame reply but found None reference")
	 return false
      end
      
      if frame.payload ~= message then
	 error ("Expected to receive different message but found: " .. frame.payload .. ", rather: " .. message)
	 return false
      end

      -- next position
      iterator = iterator + 1
   end

   -- NOTE: the test do not close conn or channe (this is intentional)

   return true
end

function test_10_create_channel (conn, channel_num, profile, received, received_data, close, close_data, user_data, next_data)
   print ("Called to create channel with profile: " .. profile)
   print ("Connection reference is: " .. tostring (conn))
   return vortex.connection.open_channel (conn, channel_num, profile)
end

function test_10 ()
   -- create context 
   ctx = vortex.ctx.new ()
   
   -- init context
   if not vortex.ctx.init (ctx) then
      print ("ERROR: expected to find proper vortex.ctx initialization..")
      return false
   end
   
   -- connect to localhost server
   print ("   Test 10: calling to create connection..")
   conn = vortex.connection.new (ctx, "localhost", "44010")
   
   -- check result
   if not vortex.connection.is_ok (conn) then
      print ("ERROR: expected to find proper connection error but found failure..")
      return false
   end

   -- create the queue
   queue = vortex.asyncqueue.new ()

   -- create a channel pool 
   pool  = conn:channel_pool_new (regression_uri, 1)
   if pool.channel_count ~= 1 then
      print ("ERROR: expected to find 1 channel in the pool but found: " .. tostring (pool.channel_count))
      return false
   end

    -- check channel pool id
    if pool.id ~= 1 then
       print ("ERROR: Expected to find channel pool id equal to 1 but found: " .. tostring (pool.id))
       return false
    end 

    print ("Checking to acquire and release channel..")
    iterator = 0
    while iterator < 10 do

        -- get a channel from the pool
       channel = pool:next_ready ()
       
       if not channel then
	  error ("Expected to find a channel reference available in the pool..but not found")
	   return false
       end

       if channel.number ~= 3 then
	  error ("Expected to find channel number 3 but found: " .. tostring (channel.number))
	  return false
       end
       
       -- check number of channels that are available at this moment
       if pool.channel_available ~= 0 then
	  error ("(1) Expected to not find any channel available but found: " .. tostring (pool.channel_available))
	  return false
       end
       
       -- ok, now release channel
       pool:release (channel)
       
       -- check number of channels that are available at this moment
       if pool.channel_available ~= 1 then
	  error ("Expected to find 1 channel available but found: " .. tostring (pool.channel_available))
	  return false
       end
       
       -- next position
       iterator = iterator + 1
    end

    info ("Checking to acquire and release channel through conn.pool() method")

    -- get a channel from the default pool
    channel = conn:pool():next_ready ()

    if not channel then
       error ("Expected to find a channel reference available in the pool..but not found")
       return false
    end

    if channel.number ~= 3 then
       error ("Expected to find channel number 3 but found: " .. tostring (channel.number))
       return false
    end

    -- check number of channels that are available at this moment
    if conn:pool().channel_available ~= 0 then
       error ("Expected to not find any channel available but found: " .. tostring (conn:pool().channel_available))
       return false
    end

    -- ok, now release channel
    conn:pool():release (channel)

    -- check number of channels that are available at this moment
    if conn:pool().channel_available ~= 1 then
       error ("Expected to find 1 channel available but found: " .. tostring (conn.pool().channel_available))
       return false
    end

    info ("Checking to acquire and release channel through conn.pool(1) method")

    -- get a channel from a particular pool
    channel = conn:pool(1):next_ready ()

    if not channel then
       error ("Expected to find a channel reference available in the pool..but not found")
       return false
    end

    if channel.number ~= 3 then
       error ("Expected to find channel number 3 but found: " .. tostring (channel.number))
       return false
    end

    -- check number of channels that are available at this moment
    if conn:pool(1).channel_available ~= 0 then
       error ("Expected to not find any channel available but found: " .. tostring (conn:pool(1).channel_available))
       return false
    end

    -- ok, now release channel
    conn:pool(1):release (channel)

    -- check number of channels that are available at this moment
    if conn:pool(1).channel_available ~= 1 then
       error ("Expected to find 1 channel available but found: " .. tostring (conn:pool(1).channel_available))
       return false
    end

    info ("Creating a new pool (using same variables), connection: " .. tostring (conn))

    -- create channel pool
    pool = conn:channel_pool_new (regression_uri, 1, test_10_create_channel, 17)

    if pool == nil then
       error ("ERROR: expected to find pool reference defined but found nil")
       return false
    end
    
    info ("Test 10: channel pool created with manual handler: " .. tostring (pool))
    
    -- check number of channels in the pool
    if pool.channel_count ~= 1 then
       error ("Expected to find channel count equal to 1 but found: " .. tostring (pool.channel_count))
       return false
    end

    -- check channel pool id
    if pool.id ~= 2 then
       error ("Expected to find channel pool id equal to 2 but found: " .. tostring (pool.id))
       return false
    end

    -- get a channel from a particular pool
    info ("Test 10: calling to get next ready (causing manual create handler to be called..)")
    channel = conn:pool(2):next_ready ()

    if not channel then
       error ("Expected to find a channel reference available in the pool..but not found")
       return false
    end

    if channel.number ~= 5 then
       error ("Expected to find channel number 5 but found: " .. tostring (channel.number))
       return false
    end

    -- release channel
    conn:pool(2):release (channel)

    info ("Now checking to access to channels from first pool..");

    -- get a channel from a particular pool
    channel = conn:pool(1):next_ready ()

    if not channel then
       error ("Expected to find a channel reference available in the pool..but not found")
       return false
    end

    if channel.number ~= 3 then
       error ("Expected to find channel number 3 but found: " .. tostring (channel.number))
       return false
    end

    -- release channel
    conn:pool(1):release (channel)

    print ("Test 10: Finished release channel from first pool")

    return true
 end

function test_11 () 
   -- create context 
   ctx = vortex.ctx.new ()
   
   -- init context
   if not vortex.ctx.init (ctx) then
      print ("ERROR: expected to find proper vortex.ctx initialization..")
      return false
   end
   
   -- connect to localhost server
   print ("   Test 11: calling to create connection..")
   conn = vortex.connection.new (ctx, "localhost", "44010")

   -- create the queue
   queue = vortex.asyncqueue.new ()

   -- open a channel
   channel = conn:open_channel (0, regression_uri_zero, queue_reply, queue)

   -- send message 
   if channel:send_msg ("Esto es una prueba...", 21) == nil then
      print ("ERROR: expected to find proper send operation..")
      return false
   end

   -- now yield
   print ("Test 11: message sent, now yield execution..")
   vortex.yield (1000000)
   
   print ("Test 11: yield finished, check frame received ..")
   frame = queue:pop ()
   
   if frame == nil then
      print ("Test 11: expected to receive frame reference defined...but found nil")
      return false
   end 

   -- check frame 
   if not vortex.frame.check (frame) then
      print ("Test 11: expected to find frame reference defined..")
      return false
   end 

   -- check frame content 
   if frame.payload ~= "Esto es una prueba..." then
      print ("ERROR: expected to find 'Esto es una prueba...' but found: " .. frame.payload)
      return false
   end

   -- now call several times to yield 
   print ("Test 11: nice, content received is ok, now call vortex.yield without no one waiting...")
   vortex.yield (1)
   vortex.yield (2)

   
   return true
end

function test_12  ()

   -- create context 
   ctx = vortex.ctx.new ()
   
   -- init context
   if not vortex.ctx.init (ctx) then
      print ("ERROR: expected to find proper vortex.ctx initialization..")
      return false
   end
   
   -- connect to localhost server
   conn = vortex.connection.new (ctx, "localhost", "44010")

   -- create the queue
   queue = vortex.asyncqueue.new ()

   -- open a channel
   channel = conn:open_channel (0, regression_uri_zero, queue_reply_unchecked, queue)

   -- send message 
   if channel:send_msg ("Esto es una prueba...", 21) == nil then
      print ("ERROR: expected to find proper send operation..")
      return false
   end

   -- send message 
   if channel:send_msg ("Esto es una prueba...", 21) == nil then
      print ("ERROR: expected to find proper send operation..")
      return false
   end

   print ("Test 12: checking to finish state (simulating a wait: 100ms)..we should finish: ")
   
   
   return true

end

function test_13_on_close (conn, data) 
   if data ~= 127 then
      error ("Expected to find 127 value but found: " .. tostring (data))
      os.exit (-1)
   end
   info ("Test 13: received connection close..")
end

function test_13  ()

   -- create event fd
   event_fd = vortex.event_fd ()

   -- create context 
   ctx = vortex.ctx.new ()
   
   -- init context
   if not vortex.ctx.init (ctx) then
      print ("ERROR: expected to find proper vortex.ctx initialization..")
      return false
   end
   
   -- connect to localhost server
   print ("   Test 13: calling to create connection..")
   conn = vortex.connection.new (ctx, "localhost", "44010")
   if not conn:is_ok () then
      error ("Test 13: failed to connect, please check server is running")
      return false
   end
   

   -- create the queue
   queue = vortex.asyncqueue.new ()

   -- open a channel
   channel = conn:open_channel (0, regression_uri, queue_reply, queue)
   if channel == nil then
      print ("Test 13: expected to find proper channel creation, but a failure was found..")
      return false
   end

   -- send message 
   print ("Test 13: sending message (generates frame received event..)")
   if channel:send_msg ("Esto es una prueba...", 21) == nil then
      print ("ERROR: expected to find proper send operation..")
      return false
   end

   print ("Test 13: get event fd for current lua state: " .. tostring (event_fd))

   print ("Test 13: waiting for global events..");
   if not vortex.wait_events () then
      error ("Test 13: expected to find wait_events () return true (events pending, but found false..")
      return false
   end 

   print ("Test 13: detected events, call to yield..")

   while vortex.wait_events (100) do 
      print ("Test 13: flusing all events....")
      vortex.yield ()
   end

   info ("Test 13: get reply..")
   frame = queue:pop ()
   if frame == nil then
      error ("Expected to find frame reference, but found nil instead")
      return false
   end

   -- check frame content 
   if frame.payload ~= "Esto es una prueba..." then
      error ("Expected to find different payload but found: " .. frame.payload)
      return false
   end
   info ("Test 13: seems we have properly received reply: " .. frame.payload)

   -- now ensure no more pending tests in place 
   if vortex.wait_events (10000) then
      error ("Expected to not find pending events but they were found..")
      return false
   end

   -- set connection close
   conn:set_on_close (test_13_on_close, 127)

   -- call to shutdown
   conn:shutdown ()

   -- ensure events are processed
   print ("Test 13: checking for events (vortex.wait_events) should be there..")
   if not vortex.wait_events () then
      error ("Expected to find connection close event, but found 0 as result from vortex.wait_events..")
      return false
   end

   print ("Test 13: check again for events after connection close..")
   while vortex.wait_events (1) do
      print ("Test 13: found events, yield..")
      vortex.yield (1)
   end

   return true

end

function test_14  ()

   -- call to initialize a context 
   ctx = vortex.ctx.new ()

   -- call to init ctx 
   if not ctx:init () then
      error ("Failed to init Vortex context")
      return false
   end

   -- call to create a connection
   conn = vortex.connection.new (ctx, "localhost", "44010")

   -- check connection status after if 
   if not conn:is_ok () then
      error ("Expected to find proper connection result, but found error. Error code was: " .. tostring(conn.status) .. ", message: " .. conn.error_msg)
      return false
   end

   -- open a channel
   channel = conn:open_channel (0, regression_uri_deny)
   if channel then
      error ("Expected to find channel error but found a proper channel reference (1)")
      return false
   end

   -- check errors here 
   err = conn:pop_channel_error ()
   if err[1] ~= 554 then
      error ("Expected to find error code 554 but found: " .. tostring (err[1]))
      return false
   end

   -- check for no more pending errors
   err = conn:pop_channel_error ()
   if err then
      error ("Expected to find nil (no error) but found: " .. err)
      return false
   end

   -- open a channel (DENY with a supported profile) 
   channel = conn:open_channel (0, regression_uri_deny_supported)
   if channel then
      error ("Expected to find channel error but found a proper channel reference (2)")
      return false
   end

   -- check errors here 
   err = conn:pop_channel_error ()
   if err[1] ~= 421 then
      error ("Expected to find error code 421 but found: " .. tostring (err[1]))
      return false
   end

   -- check for no more pending errors
   err = conn:pop_channel_error ()
   if err then
      error ("Expected to find nil (no error) but found: " .. tostring (err))
      return false
   end

   -- close connection
   conn:close ()

   -- finish context
   ctx:exit ()
   print ("Test 14: test ok, now calling again ctx:exit () to see if it breaks..")
   ctx:exit ()

   return true
end

function test_15 () 
   -- call to initialize a context 
   ctx = vortex.ctx.new ()

   -- call to init ctx 
   if not ctx:init () then
      error ("Failed to init Vortex context")
      return false
   end

   -- call to create a connection
   conn = vortex.connection.new (ctx, "localhost", "44010")

   -- check connection status after if 
   if not conn:is_ok () then
      error ("Expected to find proper connection result, but found error. Error code was then " .. tostring(conn.status) .. ", message: " .. conn.error_msg)
      return false
   end

   -- check find by uri method
   channels = conn:find_by_uri (regression_uri)
   if #channels ~= 0 then
      error ("Expected to find 0 channels opened with " .. regression_uri .. ", but found: " .. tostring (#channels))
      return false
   end

   -- now create a channel
   channel  = conn:open_channel (0, regression_uri)

   if not channel then
      error ("Expected to find proper channel creation, but error found:")
      -- get first message
      err = conn:pop_channel_error ()
      while err do
	 error ("Found error message: " .. tostring (err[1]) .. ": " .. err[2])

	 -- next message
	 err = conn:pop_channel_error ()
      end
      return false
   end

   -- check ready flag 
   if not channel.is_ready then
      error ("Expected to find channel flagged as ready..")
      return false
   end

   -- check find by uri method
   channels = conn:find_by_uri (regression_uri)
   if #channels ~= 1 then
      error ("(2) Expected to find 1 channels opened with " .. regression_uri .. ", but found: " .. tostring (#channels))
      return false
   end

   -- check channel
   if not vortex.channel.check (channels[1]) then
      error ("(3) Expected to find channel object inside channels[1] position..")
      return false
   end 

   if channels[1].number ~= channel.number then
      error ("Expected to find equal channel number, but found: " .. tostring (channels[0].number))
      return false
   end

   if channels[1].profile ~= channel.profile then
      error ("Expected to find equal channel number, but found: " .. tostring (channels[0].number))
      return false
   end

   -- check channel installed
   if conn.num_channels ~= 2 then
      error ("Expected to find only two channels installed (adminitostringative BEEP channel 0 and test channel) but found: " .. conn.num_channels ())
      return false
   end

   -- now close the channel
   print ("Test 15: now close channel..")
   if not channel:close () then
      error ("Expected to find proper channel close operation, but error found: ")
      -- get first message
      err = conn:pop_channel_error ()
      while err do
	 error ("Found error message: " .. tostring (err[1]) .. ": " .. err[2])

	 -- next message
	 err = conn:pop_channel_error ()
      end
      return false
   end

   -- check channel installed
   if conn.num_channels ~= 1 then
      error ("Expected to find only one channel installed (administrative BEEP channel 0) but found: " .. conn.num_channels)
      return false
   end
   
   return true
end

function test_16_unlock (ctx, param1, param2)
   print ("Test 16: received async event, calling to unlock..")
   vortex.unlock_listeners (ctx)
   return true -- remove this event
end

function test_16 () 
   -- call to initialize a context 
   ctx = vortex.ctx.new ()

   -- call to init ctx 
   if not ctx:init () then
      error ("Failed to init Vortex context")
      return false
   end

   -- install event to unlock afterwards
   print ("Test 16: calling to install new event..")
   print ("     " .. tostring (ctx))
   ctx:new_event (10, test_16_unlock)
   
   -- call to wait listeners
   print ("Test 16: calling to wait listeners..")
   vortex.wait_listeners (ctx)

   print ("Test 16: unlocked..")
   return true
end

function test_17 ()
   -- create a context
   ctx = vortex.ctx.new ()

   -- call to init ctx 
   if not ctx:init () then
      error ("Failed to init Vortex context")
      return false
   end

   -- connect
   conn = vortex.connection.new (ctx, "localhost", "44010")

   -- check connection status after if 
   if not conn:is_ok () then
      error ("Expected to find proper connection result, but found error. Error code was: " .. tostring(conn.status) .. ", message: " .. conn.error_msg)
      return false
   end

   -- now authenticate connection
   if not vortex.sasl.init (ctx) then
      error ("Expected to find proper authentication initialization, but found an error")
      return false
   end

   -- do an auth opeation using plain profile
   status, message = vortex.sasl.start_auth (conn, "plain", "bob", "secret")
   print ("Test 17: found status: " .. tostring (status))
   print ("Test 17: found message: " .. message)

   -- check for VortexOk status 
   if status ~= 2 then 
      error ("Expected to find VortexOk status code, but found: " .. tostring (status) .. ", error message was: " .. message)
      return false
   end
   
   -- check authentication status
   if not vortex.sasl.is_authenticated (conn) then
      error ("Expected to find is authenticated status but found un-authenticated connection")
      return false
   end

   if "http://iana.org/beep/SASL/PLAIN" ~= vortex.sasl.method_used (conn) then
      error ("Expected to find method used: http://iana.org/beep/SASL/PLAIN, but found: " .. vortex.sasl.method_used (conn))
      return false
   end

   -- check auth id 
   if vortex.sasl.auth_id (conn) ~= "bob" then
      error ("Expected to find auth id bob but found: " .. vortex.sasl.auth_id (conn))
      return false
   end

   -- close connection
   conn:close ()
   
   -- do a SASL PLAIN try with wrong crendetials
   conn = vortex.connection.new (ctx, "localhost", "44010")

   if not conn:is_ok () then
      error ("Expected to find proper connection result, but found error. Error code was: " .. tostring(conn.status) .. ", message: " .. conn.error_msg)
      return false
   end

   -- do an auth opeation using plain profile
   status, message = vortex.sasl.start_auth (conn, "plain", "bob", "secret1")

   if status ~= 1 then
      error ("Expected to find status 1 but found: " .. tostring (status))
   end
   
   -- check authentication status
   if vortex.sasl.is_authenticated (conn) then
      error ("Expected to not find is authenticated status but found un-authenticated connection")
      return false
   end

   if vortex.sasl.method_used (conn) then
      error ("Expected to find none method used but found: " .. vortex.sasl.method_used (conn))
      return false
   end

   -- check auth id 
   if vortex.sasl.auth_id (conn) then
      error ("Expected to find none auth id but found something defined: " .. vortex.sasl.auth_id (conn))
      return false
   end

   conn:close ()

   return true
end

function test_18 ()
   -- create a context
   ctx = vortex.ctx.new ()

   -- call to init ctx 
   if not ctx:init () then
      error ("Failed to init Vortex context")
      return false
   end

   -- connect
   conn = vortex.connection.new (ctx, "localhost", "44010")

   -- check connection status after if 
   if not conn:is_ok () then
      error ("Expected to find proper connection result, but found error. Error code was: " .. tostring(conn.status) .. ", message: " .. conn.error_msg)
      return false
   end

   -- now authenticate connection
   if not vortex.sasl.init (ctx) then
      error ("Expected to find proper authentication initialization, but found an error")
      return false
   end

   -- do an auth opeation using anonymous profile
   status, message = vortex.sasl.start_auth (conn, "anonymous", "test@aspl.es")

   -- check for VortexOk status 
   if status ~= 2 then 
      error ("Expected to find VortexOk status code, but found: " .. tostring (status) .. ", error message was: " .. message)
      return false
   end
   
   -- check authentication status
   if not vortex.sasl.is_authenticated (conn) then
      error ("Expected to find is authenticated status but found un-authenticated connection")
      return false
   end

   if vortex.sasl.ANONYMOUS ~= vortex.sasl.method_used (conn) then
      error ("Expected to find method used: http://iana.org/beep/SASL/ANONYMOUS, but found: " .. vortex.sasl.method_used (conn))
      return false
   end

   -- close connection
   conn:close ()
   
   -- do a SASL ANONYMOUS try with wrong crendetials
   conn = vortex.connection.new (ctx, "localhost", "44010")

   if not conn:is_ok () then
      error ("Expected to find proper connection result, but found error. Error code was: " .. tostring(conn.status) .. ", message: " .. conn.error_msg)
      return false
   end

   -- do an auth opeation using anonymous profile
   status, message = vortex.sasl.start_auth (conn, "anonymous", "wrong@aspl.es")

   if status ~= 1 then
      error ("Expected to find status 1 but found then " .. tostring (status))
      return false
   end
   
   -- check authentication status
   if vortex.sasl.is_authenticated (conn) then
      error ("Expected to not find is authenticated status but found un-authenticated connection")
      return false
   end

   if vortex.sasl.method_used (conn) then
      error ("Expected to find none method used but found: " .. vortex.sasl.method_used (conn))
      return false
   end

   return true
end

function test_19 ()
   -- create a context
   ctx = vortex.ctx.new ()

   -- call to init ctx 
   if not ctx:init () then
      error ("Failed to init Vortex context")
      return false
   end

   -- connect
   conn = vortex.connection.new (ctx, "localhost", "44010")

   -- check connection status after if 
   if not conn:is_ok () then
      error ("Expected to find proper connection result, but found error. Error code was: " .. tostring(conn.status) .. ", message: " .. conn.error_msg)
      return false
   end

   -- now authenticate connection
   if not vortex.sasl.init (ctx) then
      error ("Expected to find proper authentication initialization, but found an error")
      return false
   end

   -- do an auth opeation using DIGEST-MD5 profile
   status, message = vortex.sasl.start_auth (conn, "digest-md5", "bob", "secret", nil, "aspl.es")

   -- check for VortexOk status 
   if status ~= 2 then 
      error ("Expected to find VortexOk status code, but found: " .. tostring (status) .. ", error message was: " .. message)
      return false
   end
   
   -- check authentication status
   if not vortex.sasl.is_authenticated (conn) then
      error ("Expected to find is authenticated status but found un-authenticated connection")
      return false
   end

   if vortex.sasl.DIGEST_MD5 ~= vortex.sasl.method_used (conn) then
      error ("Expected to find method used: " .. vortex.sasl.DIGEST_MD5 .. ", but found: " .. vortex.sasl.method_used (conn))
      return false
   end

   -- check auth id 
   if vortex.sasl.auth_id (conn) ~= "bob" then
      error ("Expected to find auth id bob but found: " .. vortex.sasl.auth_id (conn))
      return false
   end

   -- close connection
   conn:close ()
   
   -- do a SASL DIGEST-MD5 try with wrong crendetials
   conn = vortex.connection.new (ctx, "localhost", "44010")

   if not conn:is_ok () then
      error ("Expected to find proper connection result, but found error. Error code was: " .. tostring(conn.status) .. ", message: " .. conn.error_msg)
      return false
   end

   -- do an auth opeation using DIGEST-MD5 profile
   status, message = vortex.sasl.start_auth (conn, "digest-md5", "bob", "secret1")

   if status ~= 1 then
      error ("Expected to find status 1 but found: " .. tostring (status))
      return false
   end
   
   -- check authentication status
   if vortex.sasl.is_authenticated (conn) then
      error ("Expected to not find is authenticated status but found un-authenticated connection")
      return false
   end

   if vortex.sasl.method_used (conn) then
      error ("Expected to find none method used but found: " .. vortex.sasl.method_used (conn))
      return false
   end

   -- check auth id 
   if vortex.sasl.auth_id (conn) then
      error ("Expected to find none auth id but found something defined: " .. vortex.sasl.auth_id (conn))
      return false
   end

   return true
end

function test_20 () 
   -- create a context
   ctx = vortex.ctx.new ()

   -- call to init ctx 
   if not ctx:init () then
      error ("Failed to init Vortex context")
      return false
   end

   -- connect
   conn = vortex.connection.new (ctx, "localhost", "44010")

   -- check connection status after if 
   if not conn:is_ok () then
      error ("Expected to find proper connection result, but found error. Error code was: " .. tostring(conn.status) .. ", message: " .. conn.error_msg)
      return false
   end

   -- now authenticate connection
   if not vortex.sasl.init (ctx) then
      error ("Expected to find proper authentication initialization, but found an error")
      return false
   end

   -- do an auth opeation using CRAM-MD5 profile
   status, message = vortex.sasl.start_auth (conn, "cram-md5", "bob", "secret")

   -- check for VortexOk status 
   if status ~= 2 then 
      error ("Expected to find VortexOk status code, but found: " .. tostring (status) .. ", error message was: " .. message)
      return false
   end
   
   -- check authentication status
   if not vortex.sasl.is_authenticated (conn) then
      error ("Expected to find is authenticated status but found un-authenticated connection")
      return false
   end

   if vortex.sasl.CRAM_MD5 ~= vortex.sasl.method_used (conn) then
      error ("Expected to find method used: " .. vortex.sasl.CRAM_MD5 .. ", but found: " .. vortex.sasl.method_used (conn))
      return false
   end

   -- check auth id 
   if vortex.sasl.auth_id (conn) ~= "bob" then
      error ("Expected to find auth id bob but found: " .. vortex.sasl.auth_id (conn))
      return false
   end

   -- close connection
   conn:close ()

   -- do a SASL CRAM-MD5 try with wrong crendetials
   conn = vortex.connection.new (ctx, "localhost", "44010")

   if not conn:is_ok () then
      error ("Expected to find proper connection result, but found error. Error code was: " .. tostring(conn.status) .. ", message: " .. conn.error_msg)
      return false
   end

   -- do an auth opeation using CRAM-MD5 profile
   status, message = vortex.sasl.start_auth (conn, "cram-md5", "bob", "secret1")

   if status ~= 1 then
      error ("Expected to find status 1 but found: " .. tostring (status))
      return false
   end

   -- check authentication status
   if vortex.sasl.is_authenticated (conn) then
      error ("Expected to not find is authenticated status but found un-authenticated connection")
      return false
   end

   if vortex.sasl.method_used (conn) then
      error ("Expected to find none method used but found: " .. vortex.sasl.method_used (conn))
      return false
   end

   -- check auth id 
   if vortex.sasl.auth_id (conn) then
      error ("Expected to find none auth id but found something defined: " .. vortex.sasl.auth_id (conn))
      return false
   end

   return true
end

function test_21 ()

   -- call to initialize a context 
   ctx = vortex.ctx.new ()

   -- call to init ctx 
   if not ctx:init () then
      error ("Failed to init Vortex context")
      return false
   end

   -- call to create a connection
   conn = vortex.connection.new (ctx, "localhost", "44010")

   -- check connection status after if 
   if not conn:is_ok () then
      error ("Expected to find proper connection result, but found error. Error code was: " .. tostring(conn.status) .. ", message: " .. conn.error_msg)
      return false
   end

   -- now create a channel
   channel = conn:open_channel (0, regression_uri)

   -- configure frame received handler 
   queue = vortex.asyncqueue.new ()
   channel:set_frame_received (function (conn, channel, frame, queue) 
				  queue:push (frame)
			       end, queue)

   -- send a message to test */
   print ("Test 21: sending utf-8 content..")
   channel:send_msg ("Camión", -1)

   -- wait for the reply
   print ("Test 21: now get content reply..")
   frame = queue:pop ()

   -- check result
   if frame.payload ~= "Camión" then
      error ("Expected to find content: Camión but found: " .. frame.payload)
      return false
   end

   -- send utf-8 content ok
   return true
end

function queue_reply_test_22  (conn, channel, frame, data)
   -- push frame into queue
   data:push (frame)
end

function test_22_common (conn)

   -- now create a channel and send content 
   channel  = conn:open_channel (0, regression_uri)

   -- flag the channel to do deliveries in a serial form
   channel:set_serialize(true)

   -- configure frame received
   queue    = vortex.asyncqueue.new ()
   channel:set_frame_received (queue_reply_test_22, queue)

   -- send 100 frames and receive its replies
   iterator = 0
   while iterator < 100 do
      -- build message
      message = ";; This buffer is for notes you don't want to save, and for Lisp evaluation.\n;; If you want to create a file, visit that file with C-x C-f,\n;; then enter the text in that file's own buffer: message num: " .. tostring (iterator)

      -- send the message
      channel:send_msg (message, #message)

      -- update iterator
      iterator = iterator + 1
   end

   info ("receiving replies..")

   -- now receive and process all messages
   iterator = 0
   while iterator < 100 do
      -- build message to check
      message = ";; This buffer is for notes you don't want to save, and for Lisp evaluation.\n;; If you want to create a file, visit that file with C-x C-f,\n;; then enter the text in that file's own buffer: message num: " .. tostring (iterator)

      -- now get a frame
      frame = queue:pop ()

      -- check content
      if frame.payload ~= message then
	 error ("Expected to find message '" .. message .. "' but found: '" .. frame.payload .. "'")
	 return false
      end

      -- next iterator
      iterator = iterator + 1
   end

   -- now check there are no pending message in the queue
   if queue:items () ~= 0 then
      error ("Expected to find 0 items in the queue but found: " .. queue:items ())
      return false
   end

   return true
end

function test_22 ()
   -- create a context
   ctx = vortex.ctx.new ()

   -- call to init ctx 
   if not ctx:init () then
      error ("Failed to init Vortex context")
      return false
   end


   -- now enable tls support on the connection
   if not vortex.tls.init (ctx) then
      error ("Expected to find proper authentication initialization, but found an error")
      return false
   end

   -- connect
   conn = vortex.connection.new (ctx, "localhost", "44010")

   -- check connection status after if 
   if not conn:is_ok () then
      error ("Expected to find proper connection result, but found error. Error code was: " .. tostring(conn.status) .. ", message: " .. conn.error_msg)
      return false
   end

   -- enable TLS on the connection 
   conn, status, status_msg = vortex.tls.start_tls (conn)

   -- check connection after tls activation
   if not conn:is_ok () then
      error ("Expected to find proper connection status after TLS activation..")
      return false
   end

   -- check status 
   if status ~= 2 then
      error ("Expected to find status code : " .. tostring (2) .. ", but found: " .. tostring (status))
      error ("Error was: " .. status_msg)
      return false
   end

   info ("TLS session activated, sending content..")
   if not test_22_common (conn) then
      return false
   end

   return true
end  

function test_23_notify (conn, status, status_msg, queue)
   -- push a tuple
   queue:push (conn)
   queue:push (status)
   queue:push (status_msg)
   return
end

function test_23 ()
   -- create a context
   ctx = vortex.ctx.new ()

   -- call to init ctx 
   if not ctx:init () then
      error ("Failed to init Vortex context")
      return false
   end

   -- now enable tls support on the connection
   if not vortex.tls.init (ctx) then
      error ("Expected to find proper authentication initialization, but found an error")
      return false
   end

   -- connect
   conn = vortex.connection.new (ctx, "localhost", "44010")

   -- check connection status after if 
   if not conn:is_ok () then
      error ("Expected to find proper connection result, but found error. Error code was: " .. tostring(conn.status) .. ", message: " .. conn.error_msg)
      return false
   end

   -- enable TLS on the connection using async notification
   queue = vortex.asyncqueue.new ()
   if vortex.tls.start_tls (conn, nil, test_23_notify, queue) then
      error ("Expected to receive None after async tls activation, but something different was found")
      return false
   end

   -- wait for the connection
   conn      = queue:pop ()
   status    = queue:pop ()
   statu_msg = queue:pop ()

   -- check connection after tls activation
   if not conn:is_ok () then
      error ("Expected to find proper connection status after TLS activation..")
      return false
   end

   -- check status 
   if status ~= 2 then
      error ("Expected to find status code : " .. tostring (vortex.status_OK) .. ", but found: " .. tostring (status))
      return false
   end

   info ("TLS session activated, sending content..")
   if not test_22_common (conn) then
      return false
   end

   return true
end

function test_24_failure_handler (conn, check_period, unreply_count, data)
    -- push connection id that failed
    conn:get_data ("test_24_queue"):push (conn.id)
    return
end

function test_24 ()

    -- all to register events
    ctx = vortex.ctx.new ()
    if not ctx:init () then
        error ("Failed to init Vortex context")
        return false
    end

    -- call to create a connection
    conn = vortex.connection.new (ctx, "localhost", "44010")
    info ("Created connection id: " .. tostring (conn.id))

    -- check connection status after if 
    if not conn:is_ok () then
        error ("Expected to find proper connection result, but found error. Error code was: " .. tostring(conn.status) .. ", message: " .. conn.error_msg)
        return false
    end

    -- configure queue
    queue = vortex.asyncqueue.new ()
    conn:set_data ("test_24_queue", queue)

    -- ok, now enable alive 
    if not vortex.alive.enable_check (conn, 20000, 10, test_24_failure_handler) then
        error ("Expect to find proper alive.enable_check but found failure..")
        return false
    end

    -- block connection for a period
    conn:block ()

    -- check that the connection is blocked
    if not conn:is_blocked () then
        error ("Expected to find blocked connection but different status..")
        return false
    end

    -- wait until failure happens
    result = queue:pop ()
    info ("Received connection closed id: " .. tostring (result))
    if result ~= conn.id then
        error ("Expected to find connection id: " .. tostring (conn.id) .. ", but found: " .. tostring (result))
        return false
    end

    info ("received connection close..")
    queue:timedpop (200000)

    -- alive check ok
    return true
end

function test_25_notify (conn, status, status_msg, queue)
    -- push status
   queue:push (status)
   queue:push (status_msg)
end

function test_25 ()
    -- create a context
    ctx = vortex.ctx.new ()

    -- call to init ctx 
    if not ctx:init () then
        error ("Failed to init Vortex context")
        return false
    end

    -- connect
    conn = vortex.connection.new (ctx, "localhost", "44010")

    -- check connection status after if 
    if not conn:is_ok () then
        error ("Expected to find proper connection result, but found error. Error code was: " .. tostring(conn.status) .. ", message: " .. conn.error_msg)
        return false
    end

    -- now authenticate connection
    if not vortex.sasl.init (ctx) then
        error ("Expected to find proper authentication initialization, but found an error")
        return false
    end

    -- do an auth opeation using plain profile
    queue = vortex.asyncqueue.new ()
    if vortex.sasl.start_auth (conn, "plain", "bob", "secret", nil, nil, test_25_notify, queue) then
        error ("Expected to find nil result but found something different..")
	return false
    end

    -- wait for reply
    status     = queue:pop ()
    status_msg = queue:pop ()

    -- check for VortexOk status 
    if status ~= 2 then 
        error ("Expected to find VortexOk status code, but found: " .. tostring (status) .. ", error message was: " .. message)
        return false
    end
        
    -- check authentication status
    if not vortex.sasl.is_authenticated (conn) then
        error ("Expected to find is authenticated status but found un-authenticated connection")
        return false
    end

    if "http://iana.org/beep/SASL/PLAIN" ~= vortex.sasl.method_used (conn) then
        error ("Expected to find method used: http://iana.org/beep/SASL/PLAIN, but found: " .. vortex.sasl.method_used (conn))
        return false
    end

    -- check auth id 
    if vortex.sasl.auth_id (conn) ~= "bob" then
        error ("Expected to find auth id bob but found: " .. vortex.sasl.auth_id (conn))
        return false
    end

    -- close connection
    conn:close ()
    
    return true
end


-- list 
test_list = {{"Test 01", test_01, "Checking context initialization and finalization"},
             {"Test 02", test_02, "Checking context initialization but not finalization"},
	     {"Test 03", test_03, "Checking we can identify a vortex.ctx"},
 	     {"Test 04", test_04, "Check BEEP basic connect support"},
	     {"Test 04-a", test_04_a, "Checking basic connection set data"},
	     {"Test 05", test_05, "Check BEEP basic connect/close support"},
             {"Test 06", test_06, "Check vortex.asyncqueue support"},
	     {"Test 07", test_07, "Check BEEP basic connect close notification (async)"}, 
             {"Test 08", test_08, "Check BEEP basic send/receive operations"}, 
             {"Test 09", test_09, "Check BEEP zeroed payload frames"}, 
             {"Test 09-a", test_09_a, "Check async channel start notification"},
             {"Test 09-b", test_09_b, "Check async channel start notification (failure expected)"},
	     {"Test 10", test_10, "Check channel pools"}, 
	     {"Test 11", test_11, "Check vortex.yield ()"},
             {"Test 12", test_12, "Check for unfinished notified handled"}, 
             {"Test 13", test_13, "Check vortex.event_fd () ()"}, 
             {"Test 14", test_14, "Check BEEP channel creation deny"}, 
	     {"Test 15", test_15, "More BEEP channels checks"},
	     {"Test 16", test_16, "Check vortex.unlock_listeners"},
             {"Test 17", test_17, "Check SASL PLAIN support"},
             {"Test 18", test_18, "Check SASL ANONYMOUS support"},
             {"Test 19", test_19, "Check DIGEST-MD5 support"},
             {"Test 20", test_20, "Check CRAM-MD5 support"},
             {"Test 21", test_21, "Check sending UTF-8 content"}, 
             {"Test 22", test_22, "Check TLS support"},
             {"Test 23", test_23, "Check TLS async notification support"}, 
             {"Test 24", test_24, "Check ALIVE support"},
	     {"Test 25", test_25, "Check SASL async notification"}}  

 
print ("Starting lua regression test..")
iterator = 1
for key, value in pairs (test_list) do
    -- get reference to the test function and description
    test_label = value[1]
    test_func = value[2]
    test_description = value[3]
    
    print (test_label .. ": - Running " .. test_description )
    if not (test_func ()) then
      print ("ERROR: found error in " .. " " .. test_description .. "\n")
      os.exit (-1)
    else
	print (test_label .. " " .. test_description .. " OK")
    end

    -- call to collect after the text
    collectgarbage ("collect")

    -- next position
    iterator = iterator + 1
end

-- call to collect after the text
collectgarbage ("collect")

print ("All test OK!")



