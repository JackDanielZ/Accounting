function toggleRow(e){
    var subRow = e.parentNode.parentNode.nextElementSibling;
    for (i = 0; i < e.getAttribute("nbChildren"); i++) {
         subRow.style.display = subRow.style.display === 'none' ? 'table-row' : 'none';
         subRow = subRow.nextElementSibling;
    }
}
