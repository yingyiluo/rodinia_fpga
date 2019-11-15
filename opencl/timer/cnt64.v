module my_cnt64(input clock,
                input resetn,
                output reg [63:0] counter);

   always @ (posedge clock)
     begin
	if (!resetn)
	  counter <= 0;
	else
	  counter <= counter + 1;
     end
endmodule

module cnt64(
            input 	  clock,
            input 	  resetn,
            input 	  ivalid,
            input 	  iready,
            output 	  ovalid,
            output 	  oready,
            input [63:0]  initval, // dummy. no input does not work
            output [63:0] counter);

   assign ovalid = 1'b1;
   assign oready = 1'b1;
   
   my_cnt64 c1(clock, resetn, counter);

endmodule // cnt64
