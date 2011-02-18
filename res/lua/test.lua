function reflect(obj, title, indent)
	indent = indent .. " "
	function dbg(k,v)
		print( indent .. " " ,typename(v), "\t", k, "\t", v, "\r\n" )	
		-- if ( typename(v) == "table" ) then reflect( v, k, indent.."  " ) end
	end

	print( indent, typename(obj), "\t: ", title, "\r\n" )	
	foreach(obj, dbg)
end

function tl()
	local t = TimeLine("TimeLine", "TimeLine.TimeLine");
		--reflect( getmetatable(t),"tl_meta"," ")
	local v1 = Value("TimeLine", "TimeLine.Value");
		--reflect( getmetatable(v1),"value_meta"," ")
	local v2 = Value("TimeLine", "TimeLine.Value");
	local s  = Sinus("TimeLine", "TimeLine.Sinus");
		--reflect( getmetatable(s),"sin_meta"," ")
	s:setName("sin0");
	v1:setName("val1");
	v1:addInput("i0");
	v2:setName("val2");
	v2:addInput("i0");
	v2:addOutput("o0");
	t:add( v1 );
	t:add( v2 );
	t:add( s );
	t:connect( "val1", "i0", "val2", "o0" );
	t:connect( "val2", "i0", "sin0", "sin" );
	--tl:resort();
	t:print();

	local v2a = t:get( "val2" );
	--reflect(getmetatable(v2a),"v2a"," ") 
	local s2a = t:get( "sin0" );
	--reflect(getmetatable(s2a),"s2a"," ") 
	return t;
end


function a()
	w = World("Core","Core.World")
	--reflect(getmetatable(w),"W"," ") 
	return w;
end

function b()
	local r = w:getRoot()
	--reflect(getmetatable(r),r:typeName()," ") 
	print("root		  : ", r:getName() )
	print("root_radius: ", r:getSphere() )
	return r;
end

function c()
	local i=100
	while(i>0) do
		f()
		i=i-1
	end
end

function d()
	local m = Mesh("Core","Core.Mesh")
	--reflect(getmetatable(m),m:typeName()," ") 
	print( m:getName(), "\r\n" )
	print( m:getTexture(2), "\r\n" )
	
	m:setPos(5,6,7);
	local x,y,z=m:getPos()
	print( m:getName(), " ", x," ", y, " ", z, "\r\n" )
	return m;
end

function e()
	local c = Camera("Core","Core.Camera")
	reflect(getmetatable(c),c:typeName()," ") 
	print( "FOv: ",c:getFov(), "\r\n" )
	print( "pos: ",c:getPos(), "\r\n" )
	print( "name: ",c:getName(), "\r\n" )
	
	c:setPos(5,6,7);
	local x,y,z = c:getPos();
	print( x,y,z, "\r\n" )
	return c;
end

function f()
	local i = io.open( "../res/lua/test.lua" )
	local s = ""
	for l in i:lines() do s = s .. l end
	i:close()
	i=0
end


function x()
	local i0,i1 = gcinfo()
	print( i0," : ",i1, "\t\t" );
	collectgarbage()
	local i0,i1 = gcinfo()
	print( i0," : ",i1, "\r\n" );
end

function v()
	print("-------REGS-------------\r\n");
	listregistry();
	print("-------GLOBALS----------\r\n");
	listglobals();
end

--a()
