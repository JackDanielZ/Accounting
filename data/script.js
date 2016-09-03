function toggleRow(e){
    var parent_row = e.parentNode.parentNode;
    var sub_row = parent_row.nextElementSibling;
    var main_level = Number(parent_row.getAttribute("level"));
    var level = Number(sub_row.getAttribute("level"));
    var parent_expanded = Boolean(parent_row.getAttribute("expanded") == "1");
    var max_depth = 1;
    var row_parity = true;
    e.textContent = parent_expanded ? "+" : "-";
    parent_row.setAttribute("expanded", parent_expanded?"0":"1");
    while (sub_row && main_level < level)
      {
         if (parent_expanded)
           {
              /* Collapse */
              sub_row.style.display = 'none';
           }
         else
           {
              var sub_expanded = Boolean(sub_row.getAttribute("expanded") == "1");
              if (sub_expanded) max_depth = level - main_level + 1;
              if (level - main_level <= max_depth)
                {
                   sub_row.style.display = 'table-row';
                   /* Set max_depth to current depth
                    * Useful after leaving an expanded row to
                    * set a correct max_depth*/
                   if (!sub_expanded) max_depth = level - main_level;
                }
           }
         sub_row = sub_row.nextElementSibling;
         level = sub_row.getAttribute("level");
      }
}
