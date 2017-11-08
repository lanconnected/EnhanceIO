#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35))
#define COMPAT_HAVE_ATOMIC64_DEC_IF_POSITIVE
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38))
#define COMPAT_HAVE_BLKDEV_GET_BY_PATH
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0))
#define COMPAT_KMAP_ATOMIC_ONE_PARAM
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0))
#define COMPAT_MAKE_REQUEST_FN_RET_VOID
#define COMPAT_MAKE_REQUEST_FN_SUBMITS_IO
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,9,0))
#define COMPAT_HAVE_WAIT_FOR_COMPLETION_IO
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0))
#define COMPAT_HAVE_STRUCT_BVEC_ITER
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,16,0))
#define COMPAT_HAVE_SMB_MB__AFTER_ATOMIC
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,17,0))
#define COMPAT_HAVE_WAIT_ON_BIT_LOCK_ACTION
#define COMPAT_WAIT_FUNCTION_HAS_PARAM
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,3,0))
#define COMPAT_HAVE_BIO_BI_ERROR
#define COMPAT_NO_BIO_GET_NR_VECS
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,4,0))
#define COMPAT_WAIT_FUNCTION_HAS_2_PARAM
#define COMPAT_MAKE_REQUEST_FN_RET_BLK_QC_T
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,8,0))
#define COMPAT_NO_GENDISK_DRIVERFS_DEV
#define COMPAT_HAVE_BIO_OPF
#endif

/*Include features backported to RedHat kernels*/
#ifdef RHEL_RELEASE_CODE
#include "compat-redhat.h"
#endif

#ifdef COMPAT_NO_GENDISK_DRIVERFS_DEV
#define EIO_DRIVERFS_DEV(GENDISK) disk_to_dev((GENDISK))->parent
#else
#define EIO_DRIVERFS_DEV(GENDISK) ((GENDISK)->driverfs_dev)
#endif

/* bio -> bi_rw/bi_opf REQ_* and BIO_RW_* REQ_OP_* compat stuff
 * Following code was mostly borrowed from DRBD driver including
 * most of comments :)
 */

#ifdef COMPAT_HAVE_BIO_OPF

#define EIO_REQ_PREFLUSH       REQ_PREFLUSH
#define EIO_REQ_FUA            REQ_FUA
#define EIO_REQ_SYNC           REQ_SYNC

        /* long gone */
#define EIO_REQ_HARDBARRIER    0
#define EIO_REQ_UNPLUG         0

        /* became an op, no longer flag */
#define EIO_REQ_DISCARD        0
#define EIO_REQ_WSAME          0

#define COMPAT_WRITE_SAME_CAPABLE

#elif defined(BIO_FLUSH)
/* RHEL 6.1 backported FLUSH/FUA as BIO_RW_FLUSH/FUA
 * and at that time also introduced the defines BIO_FLUSH/FUA.
 * There is also REQ_FLUSH/FUA, but these do NOT share
 * the same value space as the bio rw flags, yet.
 */

#define EIO_REQ_PREFLUSH       (1UL << BIO_RW_FLUSH)
#define EIO_REQ_FUA            (1UL << BIO_RW_FUA)
#define EIO_REQ_HARDBARRIER    (1UL << BIO_RW_BARRIER)
#define EIO_REQ_DISCARD        (1UL << BIO_RW_DISCARD)
#define EIO_REQ_SYNC           (1UL << BIO_RW_SYNCIO)
#define EIO_REQ_UNPLUG         (1UL << BIO_RW_UNPLUG)

#define REQ_RAHEAD             (1UL << BIO_RW_AHEAD)

#elif defined(REQ_FLUSH)        /* introduced in 2.6.36 */

#define EIO_REQ_SYNC           REQ_SYNC
#define EIO_REQ_PREFLUSH       REQ_FLUSH
#define EIO_REQ_FUA            REQ_FUA
#define EIO_REQ_DISCARD        REQ_DISCARD

#ifdef REQ_HARDBARRIER
#define EIO_REQ_HARDBARRIER    REQ_HARDBARRIER
#else
#define EIO_REQ_HARDBARRIER    0
#endif

#ifdef REQ_UNPLUG
#define EIO_REQ_UNPLUG         REQ_UNPLUG
#else
#define EIO_REQ_UNPLUG         0
#endif

#ifdef REQ_WRITE_SAME
#define EIO_REQ_WSAME          REQ_WRITE_SAME
#endif

#else                           /* Disable build on very old kernels*/
#error Kernel is too old
#endif

#ifndef EIO_REQ_WSAME
#define EIO_REQ_WSAME          0
#endif
/* END of bio -> bi_rw/bi_opf REQ_* and BIO_RW_* REQ_OP_* */


#ifndef COMPAT_HAVE_BIO_OPF
#define bi_opf bi_rw

#ifndef REQ_WRITE
/* before 2.6.36 */
#define REQ_WRITE 1
#endif

#ifdef COMPAT_BIO_OPS_BACKPORTED

#define  REQ_OP_FLUSH		REQ_OP_WRITE

#else

enum req_op {
       REQ_OP_READ,                             /* 0 */
       REQ_OP_WRITE             = REQ_WRITE,    /* 1 */

       /* Not yet a distinguished op,
        * but identified via FLUSH/FUA flags.
        * If at all. */
       REQ_OP_FLUSH             = REQ_OP_WRITE,

        /* These may be not supported in older kernels.
         * In that case, the EIO_REQ_* will be 0,
         * bio_op() aka. op_from_rq_bits() will never return these,
         * and we map the REQ_OP_* to something stupid.
         */
       REQ_OP_DISCARD           = EIO_REQ_DISCARD ?: -1,
       REQ_OP_WRITE_SAME        = EIO_REQ_WSAME   ?: -2,
};

#define bio_op(bio)          (op_from_rq_bits((bio)->bi_rw))

static inline void bio_set_op_attrs(struct bio *bio, const int op, const long flags)
{
        /* REQ_OP_READ or REQ_OP_WRITE are only supported for now.
         * (and req_op_flush, but this is req_op_write actually,
         * see above)*/
        BUG_ON(!(op == REQ_OP_READ || op == REQ_OP_WRITE));
        bio->bi_rw |= (op | flags);
}

static inline int op_from_rq_bits(u64 flags)
{
        if (flags & EIO_REQ_DISCARD)
                return REQ_OP_DISCARD;
        else if (flags & EIO_REQ_WSAME)
                return REQ_OP_WRITE_SAME;
        else if (flags & REQ_WRITE)
                return REQ_OP_WRITE;
        else
                return REQ_OP_READ;
}

#endif /* BIO_OPS_BACKPORTED */

/* This bio_flags macro is inconsistent with bio_flags in kernels > 4.8,
 * because it returns operations also, but it doesn't matter as flags
 * and operations are simply or'ed together in older kernels */
#define bio_flags(bio)       ((bio)->bi_rw)
#define submit_bio(__bio) submit_bio((__bio)->bi_rw, __bio)

#else /* COMPAT_HAVE_BIO_OPF */

/* They shuffled a way to work with bio in 4.10 AGAIN.
 * We don't have bio_flags macro.
 * We don't have WRITE_FLUSH.
 * We shouldn't use bio_set_op_attrs anymore (but we are allowed to)*/
#ifndef bio_flags
#ifndef REQ_OP_MASK /* kernel 4.8 */
#define REQ_OP_MASK	((1 << REQ_OP_BITS) - 1)
#endif
#define bio_flags(bio) ((bio)->bi_opf & ~REQ_OP_MASK)
#endif
#ifndef WRITE_FLUSH
#define WRITE_FLUSH REQ_PREFLUSH
#endif
#endif /* COMPAT_HAVE_BIO_OPF */

#ifdef COMPAT_WAIT_FUNCTION_HAS_2_PARAM
#elif defined COMPAT_WAIT_FUNCTION_HAS_PARAM
#define eio_wait_schedule(__wait_bit_key, __mode) eio_wait_schedule(__wait_bit_key)
#else
#define eio_wait_schedule(__wait_bit_key, __mode) eio_wait_schedule(void * unused)
#endif

#ifdef COMPAT_NO_BIO_GET_NR_VECS
#define EIO_BIO_GET_NR_VECS(BDEV) BIO_MAX_PAGES
#else
#define EIO_BIO_GET_NR_VECS(BDEV) bio_get_nr_vecs((BDEV))
#endif

#ifndef COMPAT_HAVE_WAIT_ON_BIT_LOCK_ACTION
#define wait_on_bit_lock_action wait_on_bit_lock
#endif

#ifndef COMPAT_HAVE_SMB_MB__AFTER_ATOMIC
#define smp_mb__after_atomic smp_mb__after_clear_bit
#endif

#ifdef COMPAT_HAVE_STRUCT_BVEC_ITER
#define EIO_BIO_BI_SECTOR(BIO) ((BIO)->bi_iter.bi_sector)
#define EIO_BIO_BI_SIZE(BIO) ((BIO)->bi_iter.bi_size)
#define EIO_BIO_BI_IDX(BIO) ((BIO)->bi_iter.bi_idx)
#else
#define EIO_BIO_BI_SECTOR(BIO) ((BIO)->bi_sector)
#define EIO_BIO_BI_SIZE(BIO) ((BIO)->bi_size)
#define EIO_BIO_BI_IDX(BIO) ((BIO)->bi_idx)
#endif

#ifdef INIT_COMPLETION /*kernels before 3.13.0*/
#define reinit_completion(__arg) INIT_COMPLETION(*(__arg))
#endif

#ifndef COMPAT_HAVE_WAIT_FOR_COMPLETION_IO
#define wait_for_completion_io(__arg) wait_for_completion(__arg)
#endif

#ifndef COMPAT_HAVE_ATOMIC64_DEC_IF_POSITIVE
static inline long atomic64_dec_if_positive(atomic64_t *v)
{
        long c, old, dec;
        c = atomic64_read(v);
        for (;;) {
                dec = c - 1;
                if (unlikely(dec < 0))
                         break;
                 old = atomic64_cmpxchg((v), c, dec);
                if (likely(old == c))
                        break;
                c = old;
        }
        return dec;
}
#endif

#ifdef COMPAT_KMAP_ATOMIC_ONE_PARAM
#define EIO_KMAP_ATOMIC(PAGE,TYPE) kmap_atomic(PAGE)
#define EIO_KUNMAP_ATOMIC(PAGE,TYPE) kunmap_atomic(PAGE)
#else
#define EIO_KMAP_ATOMIC(PAGE,TYPE) kmap_atomic(PAGE,TYPE)
#define EIO_KUNMAP_ATOMIC(PAGE,TYPE) kunmap_atomic(PAGE,TYPE)
#endif

#ifndef COMPAT_HAVE_BLKDEV_GET_BY_PATH
static inline struct block_device *blkdev_get_by_path(const char *path, fmode_t mode,
                                        void *holder)
{
        struct block_device *bdev;
        int err;

        bdev = lookup_bdev(path);
        if (IS_ERR(bdev))
                return bdev;
        err = blkdev_get(bdev, mode);
        if (err)
                return ERR_PTR(err);
       if ((mode & FMODE_WRITE) && bdev_read_only(bdev)) {
                blkdev_put(bdev, mode);
                return ERR_PTR(-EACCES);
        }
        return bdev;
}
#endif

#ifdef COMPAT_HAVE_BIO_BI_ERROR
#ifdef blk_status_to_errno /* kernels > 4.13 */
#define EIO_ENDIO_FN_START int error __maybe_unused = blk_status_to_errno(bio->bi_status)
#define EIO_BIO_ENDIO(B,E) do { (B)->bi_status = errno_to_blk_status(E); bio_endio(B); } while (0)
#else
#define EIO_ENDIO_FN_START int error __maybe_unused = bio->bi_error
#define EIO_BIO_ENDIO(B,E) do { (B)->bi_error = E; bio_endio(B); } while (0)
#endif /* blk_status_to_errno */
#define eio_endio(B,E) eio_endio(B)
#define end_unaligned_io(B,E) end_unaligned_io(B)
#define end_unaligned_free(B,E) end_unaligned_free(B)
#define eio_split_endio(B,E) eio_split_endio(B)
#define eio_bio_end_empty_barrier(B,E) eio_bio_end_empty_barrier(B)

#else
#define EIO_BIO_ENDIO(B,E) do { bio_endio(B,E); } while (0)
#define EIO_ENDIO_FN_START do {} while (0)
#endif

#ifdef COMPAT_MAKE_REQUEST_FN_RET_BLK_QC_T
#define MAKE_REQUEST_FN_TYPE blk_qc_t
#define MAKE_REQUEST_FN_RETURN_0 return BLK_QC_T_NONE
#elif defined COMPAT_MAKE_REQUEST_FN_RET_VOID
#define MAKE_REQUEST_FN_TYPE void
#define MAKE_REQUEST_FN_RETURN_0 return
#else /*return type is int for older kernels*/
#define MAKE_REQUEST_FN_TYPE int
#define MAKE_REQUEST_FN_RETURN_0 return 0
#endif
