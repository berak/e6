function print_table( t, name )
{
	if (t==null)
	{
		print("-"+name+"(nil)- \n");
		return;
	}

	local sp = " ";
	print( sp + "- "+name+" - \n");
	foreach ( x,y in t )
	{

		print( sp + x +"\t\t");
		if ( x.len()<8 )
		{
			print("\t");
		}
		print( y +"\n");
		if (type(y) == "table" || type(y) == "class" )
		{
			print_table( y,x,sp+"\t" );
		}
	}
}

