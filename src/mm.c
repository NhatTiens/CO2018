//#ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Memory management unit mm/mm.c
 */

 #include "mm.h"
 #include <stdlib.h>
 #include <stdio.h>
 
 /* 
  * init_pte - Initialize PTE entry  (Them mot muc vao bang trang)
  */
 int init_pte(uint32_t *pte, //Mot muc trong bang phan trang
              int pre,    // present (Kiem tra trang thai hien tai dang o bo nho ao khong)
              int fpn,    // FPN  (Chi so khung trang vat ly)
              int drt,    // dirty (Xac dinh du lieu da duoc chinh sua chua)
              int swp,    // swap (Trang co dang o trang thai swap)
              int swptyp, // swap type (Loai thiet bi hoan doi dang chua trang)
              int swpoff) //swap offset (Vi tri trong thiet bi hoan doi)
 {
   if (pre != 0) { // Co trong bo nho ao
     if (swp == 0) { // Non swap ~ page online
       if (fpn == 0) 
         return -1; // Invalid setting
 
       /* Valid setting with FPN */
       SETBIT(*pte, PAGING_PTE_PRESENT_MASK); // Danh dau trang hien dien
       CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK); // Xoa co swaped 
       CLRBIT(*pte, PAGING_PTE_DIRTY_MASK); // Xoa co dirty
 
       SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT); // Gan khung vat ly
     } else { // page swapped
       SETBIT(*pte, PAGING_PTE_PRESENT_MASK);// Danh dau trang hien dien
       SETBIT(*pte, PAGING_PTE_SWAPPED_MASK); // Gan co dang hoan doi
       CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);  // Xoa co dirty
 
       SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT); // Gan thiet bi hoan doi
       SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT); // Gan offset hoan doi
     }
   }
 
   return 0;   
 }
 
 /* 
  * pte_set_swap - Set PTE entry for swapped page
  * @pte    : target page table entry (PTE)
  * @swptyp : swap type
  * @swpoff : swap offset
  */
 int pte_set_swap(uint32_t *pte, int swptyp, int swpoff) // Thiet lap PTE cho trang ao bi swaped
 {	
   SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
   SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);
 
   SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
   SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);
 
   return 0;
 }
 
 /* 
  * pte_set_swap - Set PTE entry for on-line page
  * @pte   : target page table entry (PTE)
  * @fpn   : frame page number (FPN)
  */
 int pte_set_fpn(uint32_t *pte, int fpn)  // Thiet lap mot pte cho trang ao trong bo nho vat ly
 {
   SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
   CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);
 
   SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT); 
 
   return 0;
 }
 
 
 /* 
  * vmap_page_range - map a range of page at aligned address (Anh xa pham vi cac trang vao dia chi da can chinh)
  */
 int vmap_page_range(struct pcb_t *caller, // process call  (Tien trinh yeu cau anh xa)
                                 int addr, // start address which is aligned to pagesz (Dia chi bat dau)
                                int pgnum, // num of mapping page (So luong trang can anh xa)
            struct framephy_struct *frames,// list of the mapped frames (Danh sach khung trang vat ly duoc anh xa)
               struct vm_rg_struct *ret_rg)// return mapped region, the real mapped fp (Tra ve vung anh xa)
 {                                         // no guarantee all given pages are mapped
   struct framephy_struct *fpit = frames;  // Con tro di chuyen trong danh sach trang can anh xa (frames)
   int pgn = PAGING_PGN(addr);             // pgn cua trang dau tien trong danh sach (tai dia chi bat dau)
   /* TODO: update the rg_end and rg_start of ret_rg   (Tao ra 1 vung bo nho lien tuc)
   */
   ret_rg->rg_start = addr;
   ret_rg->rg_end = addr + pgnum * PAGING_PAGESZ - 1;	
     
   /* TODO map range of frame to address space 
    *      in page table pgd in caller->mm
    */
   int pgit;
   for (pgit = 0 ;pgit < pgnum; pgit ++) 
   {
     uint32_t * pte = &caller->mm->pgd[pgn + pgit];  // Lay pte cua trang hien tai (lay tu page table[pgn])
     int fpn = fpit->fpn;    // Lay so khung cua trang vat ly tu danh sach frames (fpit->fpn)
     pte_set_fpn(pte, fpn);  // Dat fpn vao pte de anh xa
     enlist_pgn_node(&caller->mm->fifo_pgn, pgn+pgit);
     fpit = fpit->fp_next;   // Di chuyen den frames tiep theo
   }
    /* Tracking for later page replacement activities (if needed)
     * Enqueue new usage page */
   return 0;
 }
 
 /* 
  * alloc_pages_range - allocate req_pgnum of frame in ram
  * @caller    : caller	(Tien trinh yeu cau cap)
  * @req_pgnum : request page num (So luong khung yeu cau)
  * @frm_lst   : frame list	 (Danh sach cac khung trang da duoc cap)
  * Danh sach cac vung nho da su dung duoc dinh nghia trong memphy_strct (struct framephy_struct *used_fp_list)
  */
 
 int alloc_pages_range(struct pcb_t *caller, int req_pgnum, struct framephy_struct** frm_lst)	
 // Cap phat mot pham vi cac khung trang trong ram
 {
   int pgit, fpn;
   struct framephy_struct *newfp_str;
   /* TODO: allocate the page 
   //caller-> ...
   //frm_lst-> ...
   */
   
   for(pgit = 0; pgit < req_pgnum; pgit++)
   {
     if(MEMPHY_get_freefp(caller->mram, &fpn) == 0)
    {
     //Lay them khung trang tu danh sach co san
      newfp_str = caller->mram->free_fp_list;
      caller->mram->free_fp_list = newfp_str->fp_next;	
     //Gan so khung trang vao newfp, va lien ket vao danh sach
     newfp_str->fpn = fpn; 
     newfp_str->owner = caller->mm;
     newfp_str->fp_next = *frm_lst;
     *frm_lst = newfp_str;
     // Them khung trang vua duoc cap phat vao dau danh sach khung tranh duoc cap cho chuong trinh
    } else {  // ERROR CODE of obtaining somes but not enough frames
       return -1;
    } 
  }
 
   return 0;
 }
 
 
 /* 
  * vm_map_ram - do the mapping all vm are to ram storage device			(Anh xa mot vung bo nho ao lien tuc vao bo nho vat ly )
  * @caller    : caller		(Tien trinh yeu cau anh xa)
  * @astart    : vm area start	(Dia chi bat dau vung bo nho can anh xa)
  * @aend      : vm area end		(Dia chi ket thuc vung bo nho can anh xa)
  * @mapstart  : start mapping point	(Diem bat dau trong bo nho vat ly noi cac bo nho ao se duoc anh xa)
  * @incpgnum  : number of mapped page	(So luong trang can anh xa)
  * @ret_rg    : returned region			(Chua thong tin ve vung bo nho ao da anh xa)
  */
 int vm_map_ram(struct pcb_t *caller, int astart, int aend, int mapstart, int incpgnum, struct vm_rg_struct *ret_rg)
 {
   struct framephy_struct *frm_lst = NULL;
   int ret_alloc;
 
   /*@bksysnet: author provides a feasible solution of getting frames
    *FATAL logic in here, wrong behaviour if we have not enough page
    *i.e. we request 1000 frames meanwhile our RAM has size of 3 frames
    *Don't try to perform that case in this simple work, it will result
    *in endless procedure of swap-off to get frame and we have not provide 
    *duplicate control mechanism, keep it simple
    */
   ret_alloc = alloc_pages_range(caller, incpgnum, &frm_lst);
     // Cap phat khung trang vat li can thiet, cac khung dung de anh xa duoc luu o frm_lst
   // frm_lst : Danh sach cac khung vat ly da duoc su dung
   if (ret_alloc < 0 && ret_alloc != -3000)
     return -1;
 
   /* Out of memory */
   if (ret_alloc == -3000) 
   {
 #ifdef MMDBG
      printf("OOM: vm_map_ram out of memory \n");
 #endif
      return -1;
   }
 
   /* it leaves the case of memory is enough but half in ram, half in swap
    * do the swaping all to swapper to get the all in ram */
   // Goi ham anh xa voi vung anh xa va vung dia chi vat ly da xac dinh truoc
   vmap_page_range(caller, mapstart, incpgnum, frm_lst, ret_rg);
 
   return 0;
 }
 
 /* Swap copy content page from source frame to destination frame  (Sao chep noi dung cua mot trang tu khung trang nguon sang dich trong qua trinh swap)
  * @mpsrc  : source memphy
  * @srcfpn : source physical page number (FPN)
  * @mpdst  : destination memphy
  * @dstfpn : destination physical page number (FPN)
  **/
 int __swap_cp_page(struct memphy_struct *mpsrc, int srcfpn,
                 struct memphy_struct *mpdst, int dstfpn) 
 {
   int cellidx;
   int addrsrc,addrdst;
   for(cellidx = 0; cellidx < PAGING_PAGESZ; cellidx++)
   {
     addrsrc = srcfpn * PAGING_PAGESZ + cellidx;
     addrdst = dstfpn * PAGING_PAGESZ + cellidx;
 
     BYTE data;
     MEMPHY_read(mpsrc, addrsrc, &data);	// Doc du lieu tu khung trang nguon
     MEMPHY_write(mpdst, addrdst, data); // Ghi du lieu vao khung trang dich
   }
 
   return 0;
 }
 
 /*
  *Initialize a empty Memory Management instance	 // Khoi tao mot quan ly bo nho trong
  * @mm:     self mm
  * @caller: mm owner
  */
 int init_mm(struct mm_struct *mm, struct pcb_t *caller)
 {
   // Tao 2 vung VMA, data hoac heap
   struct vm_area_struct * vma0 = malloc(sizeof(struct vm_area_struct));			
   struct vm_area_struct * vma1 = malloc(sizeof(struct vm_area_struct));
 
   mm->pgd = malloc(PAGING_MAX_PGN*sizeof(uint32_t));	// Cap phat bang phan trang cho trinh quan ly bo nho
   
   /* By default the owner comes with at least one vma for DATA */
   vma0->vm_id = 0;
   vma0->vm_start = -1;
   vma0->vm_end = vma0->vm_start;
   vma0->sbrk = vma0->vm_end + 1;
   vma0->vm_freerg_list=NULL;
 
   /* TODO update VMA0 next */
   // vma0->next = ...
   vma0->vm_next = vma1;
   /* TODO: update one vma for HEAP */
   vma1->vm_id = 1;
   vma1->vm_start = caller->vmemsz - 1;
   vma1->vm_end = vma1->vm_start;
   vma1->sbrk = vma1->vm_start + 1;
   vma1->vm_freerg_list = NULL;
   /* Point vma owner backward */
   // Lien ket VMA voi trinh quan ly bo nho
   vma0->vm_mm = mm; 
   vma1->vm_mm = mm;
 
   /* TODO: update mmap */
   //mm->mmap = ...
   mm->mmap = vma0;
 
   return 0;
 }
 
 struct vm_rg_struct* init_vm_rg(int rg_start, int rg_end, int vmaid)	// Khoi tao mot vung anh xa bo nho ao
 {
   // vmaid : ID cua VMA so huu vung anh xa
   struct vm_rg_struct *rgnode = malloc(sizeof(struct vm_rg_struct));
   rgnode->rg_start = rg_start;
   rgnode->rg_end = rg_end;
   rgnode->vmaid = vmaid;
   rgnode->rg_next = NULL;
 
   return rgnode;
 }
 
 int enlist_vm_rg_node(struct vm_rg_struct **rglist, struct vm_rg_struct* rgnode) // Them mot nut vung anh xa vao danh sach cac vung anh xa
 // Khi mot vung bo nho ao moi duoc tao ra va can duoc them vao danh sach quan ly cua tien trinh
 {
   // rglist : Danh sach cac vung anh xa hien co
   rgnode->rg_next = *rglist;
   *rglist = rgnode;
   
   return 0;
 }
 
 int enlist_pgn_node(struct pgn_t **plist, int pgn)
 {
   struct pgn_t* pnode = malloc(sizeof(struct pgn_t));
 
   pnode->pgn = pgn;
   pnode->pg_next = *plist;
   *plist = pnode;
 
   return 0;
 }
 
 int print_list_fp(struct framephy_struct *ifp)	 // In ra danh sach cac khung trang vat ly
 {
    struct framephy_struct *fp = ifp;
  
    printf("print_list_fp: ");
    if (fp == NULL) {printf("NULL list\n"); return -1;}
    printf("\n");
    while (fp != NULL )
    {
        printf("fp[%d]\n",fp->fpn);
        fp = fp->fp_next;
    }
    printf("\n");
    return 0;
 }
 
 int print_list_rg(struct vm_rg_struct *irg) // In ra danh sach cac  vung anh xa bo nho ao
 {
    struct vm_rg_struct *rg = irg;
  
    printf("print_list_rg: ");
    if (rg == NULL) {printf("NULL list\n"); return -1;}
    printf("\n");
    while (rg != NULL)
    {
        printf("rg[%ld->%ld<at>vma=%d](size=%ld)\n",rg->rg_start, rg->rg_end, rg->vmaid,rg->rg_end - rg->rg_start + 1);
        rg = rg->rg_next;
    }
    printf("\n");
    return 0;
 }
 
 int print_list_vma(struct vm_area_struct *ivma) // In ra danh sach cac vung bo nho ao
 {
    struct vm_area_struct *vma = ivma;
  
    printf("print_list_vma: ");
    if (vma == NULL) {printf("NULL list\n"); return -1;}
    printf("\n");
    while (vma != NULL )
    {
        printf("va[%ld->%ld]\n",vma->vm_start, vma->vm_end);
        vma = vma->vm_next;
    }
    printf("\n");
    return 0;
 }
 
 int print_list_pgn(struct pgn_t *ip)	// In ra danh sach cac so trang trong fifo
 {
    printf("print_list_pgn: ");
    if (ip == NULL) {printf("NULL list\n"); return -1;}
    printf("\n");
    while (ip != NULL )
    {
        printf("va[%d]-\n",ip->pgn);
        ip = ip->pg_next;
    }
    printf("\n");
    return 0;
 }
 
 int print_pgtbl(struct pcb_t *caller, uint32_t start, uint32_t end) // In bang trang cua mot quy trinh cu the trong mot pham vi nhat dinh
 {
   int pgn_start,pgn_end;
   int pgit;
 
   if(end == -1){
     pgn_start = 0;
     struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, 0);
     end = cur_vma->vm_end;
   }
   end++;
   pgn_start = PAGING_PGN(start);
   pgn_end = PAGING_PGN(end);
 
   printf("print_pgtbl: %d - %d", start, end);
   if (caller == NULL) {printf("NULL caller\n"); return -1;}
     printf("\n");
 
 
   for(pgit = pgn_start; pgit < pgn_end; pgit++)
   {
      printf("%08ld: %08x\n", pgit * sizeof(uint32_t), caller->mm->pgd[pgit]);
   }
   #ifdef VMDBG
   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, 1);
   if(cur_vma->vm_freerg_list == NULL) return 0;
   start = cur_vma->vm_start;
   end = cur_vma->vm_end;
   end++;
   pgn_start = PAGING_PGN(start);
   pgn_end = PAGING_PGN(end);
 
   printf("HEAP:print_pgtbl: %d - %d", start, end);
   if (caller == NULL) {printf("NULL caller\n"); return -1;}
     printf("\n");
   for(pgit = pgn_start; pgit < pgn_end; pgit++)
   {
      printf("%08ld: %08x\n", pgit * sizeof(uint32_t), caller->mm->pgd[pgit]);
   }
   #endif
 
   return 0;
 }
 
 //#endif
 