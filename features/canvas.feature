


Scenario{
     title{label __chapter__2_scenario_6 title = "Creating a canvas";};
     Given{
         let c = rt_obj(canvas{10; 20;});
     };
     Then{
        if(c.canvas.size()){
            c.canvas.width.content() == 10;
        } else 0;
     };
     And{
        c.canvas.height.content() == 20;
     };
     And{
        val every_pixel_is_black = 1;
        for(col : c.canvas.data.color){
            if(col.red.content() != 0.0){ let every_pixel_is_black = 0;}
            if(col.green.content() != 0.0){ let every_pixel_is_black = 0;}
            if(col.blue.content() != 0.0){ let every_pixel_is_black = 0;}
        }
        every_pixel_is_black;
     };
};

Scenario{
     title{label __chapter__2_scenario_6 title = "Creating a canvas";};
     Given{
         let c = rt_obj(canvas{10; 20;});
     };
};



let skip_summary = 0;
let summary_only_failures = 0;

