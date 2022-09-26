/* automatically generated from xfwm4-workspace-dialog.glade */
#ifdef __SUNPRO_C
#pragma align 4 (workspace_dialog_ui)
#endif
#ifdef __GNUC__
static const char workspace_dialog_ui[] __attribute__ ((__aligned__ (4))) =
#else
static const char workspace_dialog_ui[] =
#endif
{
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?><interface><requires lib=\"gt"
  "k+\" version=\"3.20\"/><object class=\"GtkAdjustment\" id=\"adjustment1"
  "\"><property name=\"lower\">1</property><property name=\"upper\">100</p"
  "roperty><property name=\"step_increment\">1</property><property name=\""
  "page_increment\">10</property></object><object class=\"GtkAdjustment\" "
  "id=\"adjustment2\"><property name=\"upper\">100</property><property nam"
  "e=\"step_increment\">1</property><property name=\"page_increment\">10</"
  "property></object><object class=\"GtkAdjustment\" id=\"adjustment3\"><p"
  "roperty name=\"upper\">100</property><property name=\"step_increment\">"
  "1</property><property name=\"page_increment\">10</property></object><ob"
  "ject class=\"GtkAdjustment\" id=\"adjustment4\"><property name=\"upper\""
  ">100</property><property name=\"step_increment\">1</property><property "
  "name=\"page_increment\">10</property></object><object class=\"GtkAdjust"
  "ment\" id=\"adjustment5\"><property name=\"upper\">100</property><prope"
  "rty name=\"step_increment\">1</property><property name=\"page_increment"
  "\">10</property></object><object class=\"GtkImage\" id=\"image2\"><prop"
  "erty name=\"visible\">True</property><property name=\"can_focus\">False"
  "</property><property name=\"icon_name\">window-close-symbolic</property"
  "></object><object class=\"GtkImage\" id=\"image3\"><property name=\"vis"
  "ible\">True</property><property name=\"can_focus\">False</property><pro"
  "perty name=\"icon_name\">help-browser-symbolic</property></object><obje"
  "ct class=\"XfceTitledDialog\" id=\"main-dialog\"><property name=\"can_f"
  "ocus\">False</property><property name=\"title\" translatable=\"yes\">Wo"
  "rkspaces</property><property name=\"window_position\">center-on-parent<"
  "/property><property name=\"icon_name\">org.xfce.workspaces</property><p"
  "roperty name=\"type_hint\">dialog</property><property name=\"subtitle\""
  " translatable=\"yes\">Configure layout, names and margins</property><ch"
  "ild internal-child=\"vbox\"><object class=\"GtkBox\" id=\"main-vbox\"><"
  "property name=\"visible\">True</property><property name=\"can_focus\">F"
  "alse</property><property name=\"orientation\">vertical</property><prope"
  "rty name=\"spacing\">2</property><child><object class=\"GtkNotebook\" i"
  "d=\"plug-child\"><property name=\"visible\">True</property><property na"
  "me=\"can_focus\">True</property><property name=\"border_width\">6</prop"
  "erty><child><object class=\"GtkBox\" id=\"vbox1\"><property name=\"visi"
  "ble\">True</property><property name=\"can_focus\">False</property><prop"
  "erty name=\"orientation\">vertical</property><property name=\"border_wi"
  "dth\">6</property><property name=\"spacing\">6</property><child><object"
  " class=\"GtkFrame\" id=\"frame4\"><property name=\"visible\">True</prop"
  "erty><property name=\"can_focus\">False</property><property name=\"labe"
  "l_xalign\">0</property><property name=\"shadow_type\">none</property><c"
  "hild><object class=\"GtkAlignment\" id=\"alignment4\"><property name=\""
  "visible\">True</property><property name=\"can_focus\">False</property><"
  "property name=\"top_padding\">6</property><property name=\"bottom_paddi"
  "ng\">6</property><property name=\"left_padding\">16</property><child><o"
  "bject class=\"GtkBox\" id=\"hbox3\"><property name=\"visible\">True</pr"
  "operty><property name=\"can_focus\">False</property><property name=\"or"
  "ientation\">horizontal</property><property name=\"spacing\">12</propert"
  "y><child><object class=\"GtkLabel\" id=\"label3\"><property name=\"visi"
  "ble\">True</property><property name=\"can_focus\">False</property><prop"
  "erty name=\"xalign\">0</property><property name=\"label\" translatable="
  "\"yes\">_Number of workspaces:</property><property name=\"use_underline"
  "\">True</property><property name=\"mnemonic_widget\">workspace_count_sp"
  "inbutton</property></object><packing><property name=\"expand\">False</p"
  "roperty><property name=\"fill\">True</property><property name=\"positio"
  "n\">0</property></packing></child><child><object class=\"GtkSpinButton\""
  " id=\"workspace_count_spinbutton\"><property name=\"visible\">True</pro"
  "perty><property name=\"can_focus\">True</property><property name=\"edit"
  "able\">True</property><property name=\"invisible_char\">\342\200\242</p"
  "roperty><property name=\"primary_icon_activatable\">False</property><pr"
  "operty name=\"secondary_icon_activatable\">False</property><property na"
  "me=\"primary_icon_sensitive\">True</property><property name=\"secondary"
  "_icon_sensitive\">True</property><property name=\"adjustment\">adjustme"
  "nt1</property></object><packing><property name=\"expand\">False</proper"
  "ty><property name=\"fill\">False</property><property name=\"position\">"
  "1</property></packing></child></object></child></object></child><child "
  "type=\"label\"><object class=\"GtkLabel\" id=\"label7\"><property name="
  "\"visible\">True</property><property name=\"can_focus\">False</property"
  "><property name=\"label\" translatable=\"yes\">Layout</property><proper"
  "ty name=\"use_markup\">True</property><attributes><attribute name=\"wei"
  "ght\" value=\"bold\"/></attributes></object></child></object><packing><"
  "property name=\"expand\">False</property><property name=\"fill\">True</"
  "property><property name=\"position\">0</property></packing></child><chi"
  "ld><object class=\"GtkFrame\" id=\"frame5\"><property name=\"visible\">"
  "True</property><property name=\"can_focus\">False</property><property n"
  "ame=\"label_xalign\">0</property><property name=\"shadow_type\">none</p"
  "roperty><child><object class=\"GtkAlignment\" id=\"alignment5\"><proper"
  "ty name=\"visible\">True</property><property name=\"can_focus\">False</"
  "property><property name=\"top_padding\">6</property><property name=\"le"
  "ft_padding\">18</property><child><object class=\"GtkScrolledWindow\" id"
  "=\"scrolledwindow1\"><property name=\"visible\">True</property><propert"
  "y name=\"can_focus\">True</property><property name=\"hscrollbar_policy\""
  ">never</property><property name=\"vscrollbar_policy\">automatic</proper"
  "ty><property name=\"shadow_type\">etched-in</property><child><object cl"
  "ass=\"GtkTreeView\" id=\"treeview_ws_names\"><property name=\"visible\""
  ">True</property><property name=\"can_focus\">True</property><property n"
  "ame=\"rules_hint\">True</property></object></child></object></child></o"
  "bject></child><child type=\"label\"><object class=\"GtkLabel\" id=\"lab"
  "el8\"><property name=\"visible\">True</property><property name=\"can_fo"
  "cus\">False</property><property name=\"label\" translatable=\"yes\">Nam"
  "es</property><property name=\"use_markup\">True</property><attributes><"
  "attribute name=\"weight\" value=\"bold\"/></attributes></object></child"
  "></object><packing><property name=\"expand\">True</property><property n"
  "ame=\"fill\">True</property><property name=\"position\">1</property></p"
  "acking></child></object></child><child type=\"tab\"><object class=\"Gtk"
  "Label\" id=\"label2\"><property name=\"visible\">True</property><proper"
  "ty name=\"can_focus\">False</property><property name=\"label\" translat"
  "able=\"yes\">_General</property><property name=\"use_underline\">True</"
  "property></object><packing><property name=\"tab_fill\">False</property>"
  "</packing></child><child><object class=\"GtkBox\" id=\"vbox2\"><propert"
  "y name=\"visible\">True</property><property name=\"can_focus\">False</p"
  "roperty><property name=\"orientation\">vertical</property><property nam"
  "e=\"border_width\">6</property><property name=\"spacing\">6</property><"
  "child><object class=\"GtkBox\" id=\"hbox1\"><property name=\"visible\">"
  "True</property><property name=\"can_focus\">False</property><property n"
  "ame=\"orientation\">horizontal</property><property name=\"spacing\">12<"
  "/property><child><object class=\"GtkImage\" id=\"image1\"><property nam"
  "e=\"visible\">True</property><property name=\"can_focus\">False</proper"
  "ty><property name=\"icon_name\">dialog-information-symbolic</property><"
  "property name=\"icon-size\">6</property></object><packing><property nam"
  "e=\"expand\">False</property><property name=\"fill\">True</property><pr"
  "operty name=\"position\">0</property></packing></child><child><object c"
  "lass=\"GtkLabel\" id=\"label5\"><property name=\"visible\">True</proper"
  "ty><property name=\"can_focus\">False</property><property name=\"xalign"
  "\">0</property><property name=\"label\" translatable=\"yes\">Margins ar"
  "e areas on the edges of the screen where no window will be placed</prop"
  "erty><property name=\"wrap\">True</property></object><packing><property"
  " name=\"expand\">True</property><property name=\"fill\">True</property>"
  "<property name=\"position\">1</property></packing></child></object><pac"
  "king><property name=\"expand\">False</property><property name=\"fill\">"
  "True</property><property name=\"position\">0</property></packing></chil"
  "d><child><object class=\"GtkAlignment\" id=\"alignment3\"><property nam"
  "e=\"visible\">True</property><property name=\"can_focus\">False</proper"
  "ty><property name=\"xscale\">0</property><property name=\"yscale\">0</p"
  "roperty><child><object class=\"GtkTable\" id=\"table1\"><property name="
  "\"visible\">True</property><property name=\"can_focus\">False</property"
  "><property name=\"n_rows\">3</property><property name=\"n_columns\">3</"
  "property><property name=\"column_spacing\">6</property><property name=\""
  "row_spacing\">6</property><child><placeholder/></child><child><placehol"
  "der/></child><child><object class=\"GtkSpinButton\" id=\"margin_left_sp"
  "inbutton\"><property name=\"visible\">True</property><property name=\"c"
  "an_focus\">True</property><property name=\"invisible_char\">\342\200\242"
  "</property><property name=\"primary_icon_activatable\">False</property>"
  "<property name=\"secondary_icon_activatable\">False</property><property"
  " name=\"primary_icon_sensitive\">True</property><property name=\"second"
  "ary_icon_sensitive\">True</property><property name=\"adjustment\">adjus"
  "tment2</property></object><packing><property name=\"top_attach\">1</pro"
  "perty><property name=\"bottom_attach\">2</property><property name=\"x_o"
  "ptions\"/><property name=\"y_options\"/></packing></child><child><objec"
  "t class=\"GtkSpinButton\" id=\"margin_bottom_spinbutton\"><property nam"
  "e=\"visible\">True</property><property name=\"can_focus\">True</propert"
  "y><property name=\"invisible_char\">\342\200\242</property><property na"
  "me=\"primary_icon_activatable\">False</property><property name=\"second"
  "ary_icon_activatable\">False</property><property name=\"primary_icon_se"
  "nsitive\">True</property><property name=\"secondary_icon_sensitive\">Tr"
  "ue</property><property name=\"adjustment\">adjustment3</property></obje"
  "ct><packing><property name=\"left_attach\">1</property><property name=\""
  "right_attach\">2</property><property name=\"top_attach\">2</property><p"
  "roperty name=\"bottom_attach\">3</property><property name=\"x_options\""
  "/><property name=\"y_options\"/></packing></child><child><object class="
  "\"GtkSpinButton\" id=\"margin_right_spinbutton\"><property name=\"visib"
  "le\">True</property><property name=\"can_focus\">True</property><proper"
  "ty name=\"invisible_char\">\342\200\242</property><property name=\"prim"
  "ary_icon_activatable\">False</property><property name=\"secondary_icon_"
  "activatable\">False</property><property name=\"primary_icon_sensitive\""
  ">True</property><property name=\"secondary_icon_sensitive\">True</prope"
  "rty><property name=\"adjustment\">adjustment4</property></object><packi"
  "ng><property name=\"left_attach\">2</property><property name=\"right_at"
  "tach\">3</property><property name=\"top_attach\">1</property><property "
  "name=\"bottom_attach\">2</property><property name=\"x_options\"/><prope"
  "rty name=\"y_options\"/></packing></child><child><placeholder/></child>"
  "<child><object class=\"GtkImage\" id=\"monitor_icon\"><property name=\""
  "visible\">True</property><property name=\"can_focus\">False</property><"
  "property name=\"xpad\">6</property><property name=\"ypad\">6</property>"
  "<property name=\"icon_name\">image-missing-symbolic</property><property"
  " name=\"icon-size\">6</property></object><packing><property name=\"left"
  "_attach\">1</property><property name=\"right_attach\">2</property><prop"
  "erty name=\"top_attach\">1</property><property name=\"bottom_attach\">2"
  "</property></packing></child><child><object class=\"GtkSpinButton\" id="
  "\"margin_top_spinbutton\"><property name=\"visible\">True</property><pr"
  "operty name=\"can_focus\">True</property><property name=\"invisible_cha"
  "r\">\342\200\242</property><property name=\"primary_icon_activatable\">"
  "False</property><property name=\"secondary_icon_activatable\">False</pr"
  "operty><property name=\"primary_icon_sensitive\">True</property><proper"
  "ty name=\"secondary_icon_sensitive\">True</property><property name=\"ad"
  "justment\">adjustment5</property></object><packing><property name=\"lef"
  "t_attach\">1</property><property name=\"right_attach\">2</property><pro"
  "perty name=\"x_options\"/><property name=\"y_options\"/></packing></chi"
  "ld><child><placeholder/></child><child><placeholder/></child><child><pl"
  "aceholder/></child><child><placeholder/></child></object></child></obje"
  "ct><packing><property name=\"expand\">True</property><property name=\"f"
  "ill\">True</property><property name=\"position\">1</property></packing>"
  "</child></object><packing><property name=\"position\">1</property></pac"
  "king></child><child type=\"tab\"><object class=\"GtkLabel\" id=\"label6"
  "\"><property name=\"visible\">True</property><property name=\"can_focus"
  "\">False</property><property name=\"label\" translatable=\"yes\">_Margi"
  "ns</property><property name=\"use_underline\">True</property></object><"
  "packing><property name=\"position\">1</property><property name=\"tab_fi"
  "ll\">False</property></packing></child></object><packing><property name"
  "=\"expand\">True</property><property name=\"fill\">True</property><prop"
  "erty name=\"position\">0</property></packing></child><child internal-ch"
  "ild=\"action_area\"><object class=\"GtkButtonBox\" id=\"dialog-action_a"
  "rea1\"><property name=\"visible\">True</property><property name=\"can_f"
  "ocus\">False</property><property name=\"orientation\">horizontal</prope"
  "rty><property name=\"layout_style\">end</property><child><object class="
  "\"GtkButton\" id=\"button2\"><property name=\"label\" translatable=\"ye"
  "s\">_Help</property><property name=\"use_action_appearance\">False</pro"
  "perty><property name=\"visible\">True</property><property name=\"can_fo"
  "cus\">True</property><property name=\"receives_default\">True</property"
  "><property name=\"image\">image3</property><property name=\"use_underli"
  "ne\">True</property></object><packing><property name=\"expand\">False</"
  "property><property name=\"fill\">False</property><property name=\"posit"
  "ion\">0</property><property name=\"secondary\">True</property></packing"
  "></child><child><object class=\"GtkButton\" id=\"button1\"><property na"
  "me=\"label\" translatable=\"yes\">_Close</property><property name=\"use"
  "_action_appearance\">False</property><property name=\"visible\">True</p"
  "roperty><property name=\"can_focus\">True</property><property name=\"re"
  "ceives_default\">True</property><property name=\"image\">image2</proper"
  "ty><property name=\"use_underline\">True</property></object><packing><p"
  "roperty name=\"expand\">False</property><property name=\"fill\">False</"
  "property><property name=\"pack_type\">end</property><property name=\"po"
  "sition\">1</property></packing></child></object><packing><property name"
  "=\"expand\">False</property><property name=\"fill\">True</property><pro"
  "perty name=\"pack_type\">end</property><property name=\"position\">0</p"
  "roperty></packing></child></object></child><action-widgets><action-widg"
  "et response=\"-11\">button2</action-widget><action-widget response=\"0\""
  ">button1</action-widget></action-widgets></object></interface>"
};

static const unsigned workspace_dialog_ui_length = 14583u;
